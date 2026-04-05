// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "encode.hpp"

#include "utility.hpp"

#include "support/portable_arbitrary_map.hpp"

#include <cassert>

using std::ifstream;
using std::ofstream;
using std::vector;
using std::filesystem::path;

namespace charls::cli {

namespace {

constexpr uint32_t log2_floor(const uint32_t n) noexcept
{
    ASSERT(n != 0 && "log2 is not defined for 0");

    uint32_t result{};
    uint32_t val{n};
    while ((val >>= 1UL) != 0)
        ++result;
    return result;
}

uint32_t max_value_to_bits_per_sample(const uint32_t max_value) noexcept
{
    ASSERT(max_value > 0);
    return log2_floor(max_value) + 1;
}

// Purpose: this function can encode an image stored in the Portable Anymap Format (PNM)
//          into the JPEG-LS format. The 2 binary formats P5 and P6 are supported:
//          Portable GrayMap: P5 = binary, extension = .pgm, 0-2^16 (gray scale)
//          Portable PixMap: P6 = binary, extension.ppm, range 0-2^16 (RGB)
void encode_pnm(const path& filename_input, const path& filename_output)
{
    ifstream pnm_file(open_input_stream(filename_input));

    const auto read_values{read_pnm_header(pnm_file)};
    if (read_values.size() != 4)
        throw std::runtime_error("File " + filename_input.string() + " contains a bad PNM header");

    const frame_info frame_info{static_cast<uint32_t>(read_values[1]), static_cast<uint32_t>(read_values[2]),
                                static_cast<int32_t>(max_value_to_bits_per_sample(static_cast<uint32_t>(read_values[3]))),
                                read_values[0] == 6 ? 3 : 1};

    const auto bytes_per_sample{static_cast<int32_t>(bit_to_byte_count(frame_info.bits_per_sample))};
    vector<uint8_t> input_buffer(static_cast<size_t>(frame_info.width) * frame_info.height * bytes_per_sample *
                                 frame_info.component_count);
    read(pnm_file, input_buffer);
    if (!pnm_file.good())
        throw std::runtime_error("Failed to read from file" + filename_input.string());

    // PNM format is stored with most significant byte first (big endian).
    if (bytes_per_sample == 2)
    {
        for (auto i{input_buffer.begin()}; i != input_buffer.end(); i += 2)
        {
            iter_swap(i, i + 1);
        }
    }

    jpegls_encoder encoder;
    encoder.frame_info(frame_info)
        .interleave_mode(frame_info.component_count == 3 ? interleave_mode::line : interleave_mode::none);

    vector<uint8_t> destination(encoder.estimated_destination_size());
    encoder.destination(destination);
    const size_t bytes_encoded{encoder.encode(input_buffer)};

    ofstream jls_file_stream(open_output_stream(filename_output));
    write(jls_file_stream, destination, bytes_encoded);
    jls_file_stream.close();
}


void encode_pam(const path& filename_input, const path& filename_output)
{
    const support::portable_arbitrary_map pam_file(filename_input);

    const frame_info frame_info{pam_file.width(), pam_file.height(), pam_file.bits_per_sample(), pam_file.component_count()};

    jpegls_encoder encoder;
    encoder.frame_info(frame_info)
        .interleave_mode(frame_info.component_count > 1 ? interleave_mode::line : interleave_mode::none);

    vector<uint8_t> destination(encoder.estimated_destination_size());
    encoder.destination(destination);
    const size_t bytes_encoded{encoder.encode(pam_file.image_data())};

    ofstream jls_file_stream(open_output_stream(filename_output));
    write(jls_file_stream, destination, bytes_encoded);
    jls_file_stream.close();
}

} // namespace


void encode_netpbm(const path& filename_input, const path& filename_output)
{
    if (filename_input.extension() == ".pam")
    {
        encode_pam(filename_input, filename_output);
    }
    else
    {
        encode_pnm(filename_input, filename_output);
    }
}

} // namespace charls::cli
