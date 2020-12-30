// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include "../src/jpeg_stream_writer.h"

#include "../test/portable_anymap_file.h"

#include <random>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::ifstream;
using std::ios;
using std::mt19937;
using std::uniform_int_distribution;
using std::vector;
using namespace charls_test;

namespace {

void triplet_to_planar(vector<uint8_t>& buffer, const uint32_t width, const uint32_t height)
{
    vector<uint8_t> work_buffer(buffer.size());

    const size_t byte_count{static_cast<size_t>(width) * height};
    for (size_t index{}; index < byte_count; index++)
    {
        work_buffer[index] = buffer[index * 3 + 0];
        work_buffer[index + 1 * byte_count] = buffer[index * 3 + 1];
        work_buffer[index + 2 * byte_count] = buffer[index * 3 + 2];
    }
    swap(buffer, work_buffer);
}

} // namespace

namespace charls { namespace test {

vector<uint8_t> read_file(const char* filename)
{
    ifstream input;
    input.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    input.open(filename, ios::in | ios::binary);

    input.seekg(0, ios::end);
    const auto byte_count_file = static_cast<size_t>(input.tellg());
    input.seekg(0, ios::beg);

    vector<uint8_t> buffer(byte_count_file);
    input.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

    return buffer;
}

portable_anymap_file read_anymap_reference_file(const char* filename, const interleave_mode interleave_mode,
                                                const frame_info& frame_info)
{
    portable_anymap_file reference_file{filename};

    if (interleave_mode == interleave_mode::none && frame_info.component_count == 3)
    {
        triplet_to_planar(reference_file.image_data(), frame_info.width, frame_info.height);
    }

    return reference_file;
}

portable_anymap_file read_anymap_reference_file(const char* filename, const interleave_mode interleave_mode)
{
    portable_anymap_file reference_file{filename};

    if (interleave_mode == interleave_mode::none && reference_file.component_count() == 3)
    {
        triplet_to_planar(reference_file.image_data(), reference_file.width(), reference_file.height());
    }

    return reference_file;
}

vector<uint8_t> create_test_spiff_header(const uint8_t high_version, const uint8_t low_version, const bool end_of_directory)
{
    vector<uint8_t> buffer;
    buffer.push_back(0xFF);
    buffer.push_back(0xD8); // SOI.
    buffer.push_back(0xFF);
    buffer.push_back(0xE8); // ApplicationData8
    buffer.push_back(0);
    buffer.push_back(32);

    // SPIFF identifier string.
    buffer.push_back('S');
    buffer.push_back('P');
    buffer.push_back('I');
    buffer.push_back('F');
    buffer.push_back('F');
    buffer.push_back(0);

    // Version
    buffer.push_back(high_version);
    buffer.push_back(low_version);

    buffer.push_back(0); // profile id
    buffer.push_back(3); // component count

    // Height
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0x3);
    buffer.push_back(0x20);

    // Width
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0x2);
    buffer.push_back(0x58);

    buffer.push_back(10); // color space
    buffer.push_back(8);  // bits per sample
    buffer.push_back(6);  // compression type, 6 = JPEG-LS
    buffer.push_back(1);  // resolution units

    // vertical_resolution
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(96);

    // header.horizontal_resolution = 1024;
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(4);
    buffer.push_back(0);

    const size_t spiff_header_size{buffer.size()};
    buffer.resize(buffer.size() + 100);

    jpeg_stream_writer writer({buffer.data() + spiff_header_size, buffer.size() - spiff_header_size});

    if (end_of_directory)
    {
        writer.write_spiff_end_of_directory_entry();
    }

    writer.write_start_of_frame_segment(100, 100, 8, 1);
    writer.write_start_of_scan_segment(1, 0, interleave_mode::none);

    return buffer;
}

vector<uint8_t> create_noise_image_16_bit(const size_t pixel_count, const int bit_count, const uint32_t seed)
{
    const auto max_value = static_cast<uint16_t>((1U << bit_count) - 1U);
    mt19937 generator(seed);

    MSVC_CONST uniform_int_distribution<uint16_t> distribution(0, max_value);

    vector<uint8_t> buffer(pixel_count * 2);
    for (size_t i{}; i < pixel_count; i = i + 2)
    {
        const uint16_t value{distribution(generator)};

        buffer[i] = static_cast<uint8_t>(value);
        buffer[i] = static_cast<uint8_t>(value >> 8);
    }
    return buffer;
}

void test_round_trip_legacy(const vector<uint8_t>& source, const JlsParameters& params)
{
    // ReSharper disable CppDeprecatedEntity
    DISABLE_DEPRECATED_WARNING

    vector<uint8_t> encoded_buffer(params.height * params.width * params.components * params.bitsPerSample / 4);
    vector<uint8_t> decoded_buffer(static_cast<size_t>(params.height) * params.width *
                                   bit_to_byte_count(params.bitsPerSample) * params.components);

    size_t compressed_length{};
    auto error{JpegLsEncode(encoded_buffer.data(), encoded_buffer.size(), &compressed_length, source.data(), source.size(),
                            &params, nullptr)};
    Assert::AreEqual(jpegls_errc::success, error);

    error = JpegLsDecode(decoded_buffer.data(), decoded_buffer.size(), encoded_buffer.data(), compressed_length, nullptr,
                         nullptr);
    Assert::AreEqual(jpegls_errc::success, error);

    const uint8_t* byte_out{decoded_buffer.data()};
    for (size_t i{}; i < decoded_buffer.size(); ++i)
    {
        if (source[i] != byte_out[i])
        {
            Assert::IsTrue(false);
            break;
        }
    }

    // ReSharper restore CppDeprecatedEntity
    RESTORE_DEPRECATED_WARNING
}

}} // namespace charls::test
