// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "encode.hpp"
#include "utility.hpp"

#include "support/portable_arbitrary_map.hpp"
#include <charls/jpegls_encoder.hpp>

#include <algorithm>
#include <cassert> // IWYU pragma: keep
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

using std::byte;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::filesystem::path;

namespace charls::cli {

namespace {

[[nodiscard]]
constexpr uint32_t log2_floor(const uint32_t n) noexcept
{
    ASSERT(n != 0 && "log2 is not defined for 0");

    uint32_t result{};
    uint32_t val{n};
    while ((val >>= 1UL) != 0)
        ++result;
    return result;
}

[[nodiscard]]
uint32_t max_value_to_bits_per_sample(const uint32_t max_value) noexcept
{
    ASSERT(max_value > 0);
    return log2_floor(max_value) + 1;
}

[[nodiscard]]
interleave_mode determine_interleave_mode(const int32_t interleave_mode, const frame_info& frame_info)
{
    if (interleave_mode == default_interleave_mode)
    {
        return frame_info.component_count > 1 ? interleave_mode::line : interleave_mode::none;
    }

    return static_cast<charls::interleave_mode>(interleave_mode);
}

template<typename SizeType>
void convert_rgb_to_planar(const size_t width, const size_t height, const void* source, void* destination) noexcept
{
    const auto* rgb{static_cast<const SizeType*>(source)};

    const size_t planar_size{width * height};
    auto* r{static_cast<SizeType*>(destination)};
    auto* g{static_cast<SizeType*>(destination) + planar_size};
    auto* b{static_cast<SizeType*>(destination) + (planar_size * 2)};

    for (size_t row{}; row != height; ++row)
    {
        for (size_t col{}, offset = 0; col != width; ++col, offset += 3)
        {
            r[col] = rgb[offset + 0];
            g[col] = rgb[offset + 1];
            b[col] = rgb[offset + 2];
        }

        r += width;
        g += width;
        b += width;
        rgb += width * 3;
    }
}

void encode(vector<byte>& source, const frame_info& frame_info, const size_t bytes_per_sample, const path& filename_output,
            const int32_t interleave_mode, const int32_t near_lossless, const int32_t color_transformation)
{
    jpegls_encoder encoder;
    encoder.frame_info(frame_info)
        .near_lossless(near_lossless)
        .color_transformation(static_cast<charls::color_transformation>(color_transformation));

    const auto final_interleave_mode = determine_interleave_mode(interleave_mode, frame_info);
    encoder.interleave_mode(final_interleave_mode);
    if (final_interleave_mode == interleave_mode::none && frame_info.component_count == 3)
    {
        vector<byte> planar_buffer(source.size());
        if (bytes_per_sample == 2)
        {
            convert_rgb_to_planar<uint16_t>(frame_info.width, frame_info.height, source.data(), planar_buffer.data());
        }
        else
        {
            convert_rgb_to_planar<byte>(frame_info.width, frame_info.height, source.data(), planar_buffer.data());
        }
        std::swap(source, planar_buffer);
    }

    vector<byte> destination(encoder.estimated_destination_size());
    encoder.destination(destination);
    const size_t bytes_encoded{encoder.encode(source)};

    ofstream jls_file_stream(open_output_stream(filename_output));
    write(jls_file_stream, destination, bytes_encoded);
    jls_file_stream.close();
}

// Purpose: this function can encode an image stored in the Portable Anymap Format (PNM)
//          into the JPEG-LS format. The 2 binary formats P5 and P6 are supported:
//          Portable GrayMap: P5 = binary, extension = .pgm, 0-2^16 (gray scale)
//          Portable PixMap: P6 = binary, extension.ppm, range 0-2^16 (RGB)
void encode_pnm(const path& filename_input, const path& filename_output, const int32_t interleave_mode,
                const int32_t near_lossless, const int32_t color_transformation)
{
    ifstream pnm_file(open_input_stream(filename_input));

    const auto read_values{read_pnm_header(pnm_file)};
    if (read_values.size() != 4)
        throw std::runtime_error("File " + filename_input.string() + " contains a bad PNM header");

    const frame_info frame_info{static_cast<uint32_t>(read_values[1]), static_cast<uint32_t>(read_values[2]),
                                static_cast<int32_t>(max_value_to_bits_per_sample(static_cast<uint32_t>(read_values[3]))),
                                read_values[0] == 6 ? 3 : 1};

    const auto bytes_per_sample{static_cast<size_t>(bit_to_byte_count(frame_info.bits_per_sample))};
    vector<byte> input_buffer(bytes_per_sample * frame_info.width * frame_info.height * frame_info.component_count);
    read(pnm_file, input_buffer);
    if (!pnm_file.good())
        throw std::runtime_error("Failed to read from file " + filename_input.string());

    // PNM format is stored with most significant byte first (big endian).
    if (bytes_per_sample == 2)
    {
        for (auto i{input_buffer.begin()}; i != input_buffer.end(); i += 2)
        {
            iter_swap(i, i + 1);
        }
    }

    encode(input_buffer, frame_info, bytes_per_sample, filename_output, interleave_mode, near_lossless, color_transformation);
}

void encode_pam(const path& filename_input, const path& filename_output, const int32_t interleave_mode,
                const int32_t near_lossless, const int32_t color_transformation)
{
    support::portable_arbitrary_map pam_file(filename_input);

    const frame_info frame_info{pam_file.width(), pam_file.height(), pam_file.bits_per_sample(), pam_file.component_count()};
    const auto bytes_per_sample{static_cast<size_t>(bit_to_byte_count(frame_info.bits_per_sample))};

    encode(pam_file.image_data(), frame_info, bytes_per_sample, filename_output, interleave_mode, near_lossless,
           color_transformation);
}

} // namespace


void encode_netpbm(const path& filename_input, const path& filename_output, const int32_t interleave_mode,
                   const int32_t near_lossless, const int32_t color_transformation)
{
    if (filename_input.extension() == ".pam")
    {
        encode_pam(filename_input, filename_output, interleave_mode, near_lossless, color_transformation);
    }
    else
    {
        encode_pnm(filename_input, filename_output, interleave_mode, near_lossless, color_transformation);
    }
}

} // namespace charls::cli
