// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "decode.hpp"

#include "utility.hpp"

using std::byte;
using std::ifstream;
using std::ios;
using std::istream;
using std::ofstream;
using std::runtime_error;
using std::vector;
using std::filesystem::path;

namespace charls::cli {

namespace {

template<typename SizeType>
void convert_planar_to_pixel(const size_t width, const size_t height, const void* source, void* destination) noexcept
{
    const size_t stride_in_pixels{width * 3};
    const auto* plane0{static_cast<const SizeType*>(source)};
    const auto* plane1{static_cast<const SizeType*>(source) + (width * height)};
    const auto* plane2{static_cast<const SizeType*>(source) + (width * height * 2)};

    auto* pixels{static_cast<SizeType*>(destination)};

    for (size_t row{}; row != height; ++row)
    {
        for (size_t column{}, offset = 0; column != width; ++column, offset += 3)
        {
            pixels[offset + 0] = plane0[column];
            pixels[offset + 1] = plane1[column];
            pixels[offset + 2] = plane2[column];
        }

        plane0 += width;
        plane1 += width;
        plane2 += width;
        pixels += stride_in_pixels;
    }
}

[[nodiscard]]
size_t get_stream_length(istream& stream)
{
    stream.seekg(0, ios::end);
    const auto length{static_cast<size_t>(stream.tellg())};
    stream.seekg(0, ios::beg);

    return length;
}

} // namespace


void decode_to_pnm(const path& filename_input, const path& filename_output)
{
    ifstream input{open_input_stream(filename_input)};

    const size_t length{get_stream_length(input)};
    vector<byte> encoded_source(length);
    read(input, encoded_source);

    vector<byte> decoded_destination;
    const auto& [frame_info, interleave_mode] = jpegls_decoder::decode(encoded_source, decoded_destination);

    if (frame_info.component_count != 1 && frame_info.component_count != 3)
        throw runtime_error("Only JPEG-LS images with component count 1 or 3 are supported to decode to pnm");
    
    // PPM format only supports by-pixel, convert if needed.
    if (interleave_mode == interleave_mode::none && frame_info.component_count == 3)
    {
        vector<byte> pixels(decoded_destination.size());
        if (frame_info.bits_per_sample > 8)
        {
            convert_planar_to_pixel<uint16_t>(frame_info.width, frame_info.height, decoded_destination.data(),
                                              pixels.data());
        }
        else
        {
            convert_planar_to_pixel<uint8_t>(frame_info.width, frame_info.height, decoded_destination.data(), pixels.data());
        }

        swap(decoded_destination, pixels);
    }

    // PNM format requires most significant byte first (big endian).
    const int max_value{(1 << frame_info.bits_per_sample) - 1};

    if (const int bytes_per_sample{max_value > 255 ? 2 : 1}; bytes_per_sample == 2)
    {
        for (auto i{decoded_destination.begin()}; i != decoded_destination.end(); i += 2)
        {
            iter_swap(i, i + 1);
        }
    }

    const int magic_number{frame_info.component_count == 3 ? 6 : 5};

    ofstream output{open_output_stream(filename_output)};
    output << 'P' << magic_number << "\n" << frame_info.width << ' ' << frame_info.height << "\n" << max_value << "\n";
    write(output, decoded_destination, decoded_destination.size());
    output.close();
}

} // namespace charls::cli
