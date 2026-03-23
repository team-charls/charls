// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "support.hpp"

#include <support/portable_anymap_file.hpp>

#include "../src/jpeg_stream_writer.hpp"
#include <charls/jpegls_decoder.hpp>
#include <charls/jpegls_encoder.hpp>

#include <fstream>

using std::byte;
using std::ifstream;
using std::vector;
using namespace charls::support;

namespace charls::test {

namespace {

void triplet_to_planar(vector<byte>& triplet_buffer, const uint32_t width, const uint32_t height)
{
    vector<byte> planar_buffer(triplet_buffer.size());

    const size_t byte_count{static_cast<size_t>(width) * height};
    for (size_t index{}; index != byte_count; index++)
    {
        planar_buffer[index] = triplet_buffer[index * 3 + 0];
        planar_buffer[index + 1 * byte_count] = triplet_buffer[index * 3 + 1];
        planar_buffer[index + 2 * byte_count] = triplet_buffer[index * 3 + 2];
    }
    swap(triplet_buffer, planar_buffer);
}

} // namespace


[[nodiscard]]
vector<byte> read_file(const char* filename)
{
    std::ifstream input;
    input.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    input.open(filename, std::ios::in | std::ios::binary);

    input.seekg(0, std::ios::end);
    const auto byte_count_file{static_cast<size_t>(input.tellg())};
    input.seekg(0, std::ios::beg);

    vector<byte> buffer(byte_count_file);
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
        triplet_to_planar(reference_file.image_data(), static_cast<uint32_t>(reference_file.width()),
                          static_cast<uint32_t>(reference_file.height()));
    }

    return reference_file;
}

void decode_encode_file(const char* encoded_filename, const char* raw_filename, const bool check_encode)
{
    const auto encoded_source{read_file(encoded_filename)};
    const jpegls_decoder decoder{encoded_source, true};

    portable_anymap_file reference_file{
        read_anymap_reference_file(raw_filename, decoder.get_interleave_mode(), decoder.frame_info())};

    test_compliance(encoded_source, reference_file.image_data(), check_encode);
}

[[nodiscard]]
std::vector<std::byte> create_test_spiff_header(const uint8_t high_version, const uint8_t low_version,
                                                const bool end_of_directory, const uint8_t component_count)
{
    using std::byte;

    std::vector<byte> buffer;
    buffer.push_back(byte{0xFF});
    buffer.push_back(byte{0xD8}); // SOI.
    buffer.push_back(byte{0xFF});
    buffer.push_back(byte{0xE8}); // ApplicationData8
    buffer.push_back({});
    buffer.push_back(byte{32});

    // SPIFF identifier string.
    buffer.push_back(byte{'S'});
    buffer.push_back(byte{'P'});
    buffer.push_back(byte{'I'});
    buffer.push_back(byte{'F'});
    buffer.push_back(byte{'F'});
    buffer.push_back({});

    // Version
    buffer.push_back(byte{high_version});
    buffer.push_back(byte{low_version});

    buffer.push_back({}); // profile id
    buffer.push_back(byte{component_count});

    // Height
    buffer.push_back({});
    buffer.push_back({});
    buffer.push_back(byte{0x3});
    buffer.push_back(byte{0x20});

    // Width
    buffer.push_back({});
    buffer.push_back({});
    buffer.push_back(byte{0x2});
    buffer.push_back(byte{0x58});

    buffer.push_back(byte{10}); // color space
    buffer.push_back(byte{8});  // bits per sample
    buffer.push_back(byte{6});  // compression type, 6 = JPEG-LS
    buffer.push_back(byte{1});  // resolution units

    // vertical_resolution
    buffer.push_back({});
    buffer.push_back({});
    buffer.push_back({});
    buffer.push_back(byte{96});

    // header.horizontal_resolution = 1024
    buffer.push_back({});
    buffer.push_back({});
    buffer.push_back(byte{4});
    buffer.push_back({});

    const size_t spiff_header_size{buffer.size()};
    buffer.resize(buffer.size() + 100);

    jpeg_stream_writer writer;
    writer.destination({buffer.data() + spiff_header_size, buffer.size() - spiff_header_size});

    if (end_of_directory)
    {
        writer.write_spiff_end_of_directory_entry();
    }

    writer.write_start_of_frame_segment({600, 800, 8, 3});
    writer.write_start_of_scan_segment(1, 0, interleave_mode::none);

    return buffer;
}

bool verify_encoded_bytes(const vector<byte>& uncompressed_source, const vector<byte>& encoded_source)
{
    const jpegls_decoder decoder{encoded_source, true};

    jpegls_encoder encoder;
    encoder.frame_info(decoder.frame_info())
        .interleave_mode(decoder.get_interleave_mode())
        .near_lossless(decoder.get_near_lossless())
        .preset_coding_parameters(decoder.preset_coding_parameters());

    vector<byte> our_encoded_bytes(encoded_source.size() + 16);
    encoder.destination(our_encoded_bytes);

    if (const size_t bytes_written{encoder.encode(uncompressed_source)}; bytes_written != encoded_source.size())
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

void test_compliance(const vector<byte>& encoded_source, const vector<byte>& uncompressed_source, const bool check_encode)
{
    if (check_encode)
    {
        ASSERT_TRUE(verify_encoded_bytes(uncompressed_source, encoded_source));
    }

    jpegls_decoder decoder{encoded_source, true};
    const auto destination{decoder.decode<vector<byte>>()};

    if (decoder.get_near_lossless() == 0)
    {
        for (size_t i{}; i != uncompressed_source.size(); ++i)
        {
            if (uncompressed_source[i] != destination[i]) // AreEqual is very slow, pre-test to speed up 50X
            {
                ASSERT_EQ(uncompressed_source[i], destination[i]);
            }
        }
    }
    else
    {
        const frame_info frame_info{decoder.frame_info()};
        const auto near_lossless{decoder.get_near_lossless()};

        if (frame_info.bits_per_sample <= 8)
        {
            for (size_t i{}; i != uncompressed_source.size(); ++i)
            {
                if (abs(static_cast<int>(uncompressed_source[i]) - static_cast<int>(destination[i])) >
                    near_lossless) // AreEqual is very slow, pre-test to speed up 50X
                {
                    ASSERT_EQ(uncompressed_source[i], destination[i]);
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
                    ASSERT_EQ(static_cast<int>(source16[i]), static_cast<int>(destination16[i]));
                }
            }
        }
    }
}


} // namespace charls::test
