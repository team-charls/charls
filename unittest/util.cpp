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

namespace charls { namespace test {

namespace {

void triplet_to_planar(vector<uint8_t>& buffer, const uint32_t width, const uint32_t height)
{
    vector<uint8_t> work_buffer(buffer.size());

    const size_t byte_count{static_cast<size_t>(width) * height};
    for (size_t index{}; index != byte_count; index++)
    {
        work_buffer[index] = buffer[index * 3 + 0];
        work_buffer[index + 1 * byte_count] = buffer[index * 3 + 1];
        work_buffer[index + 2 * byte_count] = buffer[index * 3 + 2];
    }
    swap(buffer, work_buffer);
}

} // namespace


vector<uint8_t> read_file(const char* filename)
{
    ifstream input;
    input.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    input.open(filename, ios::in | ios::binary);

    input.seekg(0, ios::end);
    const auto byte_count_file{static_cast<size_t>(input.tellg())};
    input.seekg(0, ios::beg);

    vector<uint8_t> buffer(byte_count_file);
    input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));

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

vector<uint8_t> create_test_spiff_header(const uint8_t high_version, const uint8_t low_version, const bool end_of_directory,
                                         const uint8_t component_count)
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
    buffer.push_back(component_count);

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

    // header.horizontal_resolution = 1024
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

    writer.write_start_of_frame_segment({600, 800, 8, 3});
    writer.write_start_of_scan_segment(1, 0, interleave_mode::none);

    return buffer;
}

vector<uint8_t> create_noise_image_16_bit(const size_t pixel_count, const int bit_count, const uint32_t seed)
{
    const auto max_value{static_cast<uint16_t>((1U << bit_count) - 1U)};
    mt19937 generator(seed);

    MSVC_WARNING_SUPPRESS_NEXT_LINE(26496) // cannot be marked as const as operator() is not always defined const.
    uniform_int_distribution<uint16_t> distribution(0, max_value);

    vector<uint8_t> buffer(pixel_count * 2);
    for (size_t i{}; i != pixel_count; i = i + 2)
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
    for (size_t i{}; i != decoded_buffer.size(); ++i)
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

bool verify_encoded_bytes(const vector<uint8_t>& uncompressed_source, const vector<uint8_t>& encoded_source)
{
    const jpegls_decoder decoder{encoded_source, true};

    jpegls_encoder encoder;
    encoder.frame_info(decoder.frame_info())
        .interleave_mode(decoder.interleave_mode())
        .near_lossless(decoder.near_lossless())
        .preset_coding_parameters(decoder.preset_coding_parameters());

    vector<uint8_t> our_encoded_bytes(encoded_source.size() + 16);
    encoder.destination(our_encoded_bytes);

    const size_t bytes_written{encoder.encode(uncompressed_source)};
    if (bytes_written != encoded_source.size())
        return false;

    for (size_t i{}; i != encoded_source.size(); ++i)
    {
        if (encoded_source[i] != our_encoded_bytes[i])
        {
            return false;
        }
    }

    return true;
}

void verify_decoded_bytes(const interleave_mode interleave_mode, const frame_info& frame_info,
                          const vector<uint8_t>& uncompressed_data, const size_t destination_stride,
                          const char* reference_filename)
{
    const auto anymap_reference{read_anymap_reference_file(reference_filename, interleave_mode, frame_info)};
    const auto& reference_samples{anymap_reference.image_data()};

    const int plane_count{interleave_mode == interleave_mode::none ? frame_info.component_count : 1};
    const int components_in_plane_count{interleave_mode == interleave_mode::none ? 1 : frame_info.component_count};

    const size_t source_stride{static_cast<size_t>(frame_info.width) * components_in_plane_count};
    const uint8_t* sample{uncompressed_data.data()};
    size_t reference_sample{};
    for (int plane{}; plane < plane_count; ++plane)
    {
        for (uint32_t line{}; line < frame_info.height; ++line)
        {
            for (size_t i{}; i < source_stride; ++i)
            {
                if (sample[i] != reference_samples[reference_sample]) // AreEqual is very slow, pre-test to speed up 50X
                {
                    Assert::AreEqual(sample[i], reference_samples[reference_sample]);
                }
                ++reference_sample;
            }

            sample += destination_stride;
        }
    }
}

void test_compliance(const vector<uint8_t>& encoded_source, const vector<uint8_t>& uncompressed_source,
                     const bool check_encode)
{
    const jpegls_decoder decoder{encoded_source, true};

    if (check_encode)
    {
        Assert::IsTrue(verify_encoded_bytes(uncompressed_source, encoded_source));
    }

    vector<uint8_t> destination(decoder.destination_size());
    decoder.decode(destination);

    if (decoder.near_lossless() == 0)
    {
        for (size_t i{}; i != uncompressed_source.size(); ++i)
        {
            if (uncompressed_source[i] != destination[i]) // AreEqual is very slow, pre-test to speed up 50X
            {
                Assert::AreEqual(uncompressed_source[i], destination[i]);
            }
        }
    }
    else
    {
        const frame_info frame_info{decoder.frame_info()};
        const auto near_lossless{decoder.near_lossless()};

        if (frame_info.bits_per_sample <= 8)
        {
            for (size_t i{}; i != uncompressed_source.size(); ++i)
            {
                if (abs(uncompressed_source[i] - destination[i]) >
                    near_lossless) // AreEqual is very slow, pre-test to speed up 50X
                {
                    Assert::AreEqual(uncompressed_source[i], destination[i]);
                }
            }
        }
        else
        {
            const void* data{uncompressed_source.data()};
            const auto* source16{static_cast<const uint16_t*>(data)};
            data = destination.data();
            const auto* destination16{static_cast<const uint16_t*>(data)};

            for (size_t i{}; i != uncompressed_source.size() / 2; ++i)
            {
                if (abs(source16[i] - destination16[i]) > near_lossless) // AreEqual is very slow, pre-test to speed up 50X
                {
                    Assert::AreEqual(static_cast<int>(source16[i]), static_cast<int>(destination16[i]));
                }
            }
        }
    }
}


}} // namespace charls::test
