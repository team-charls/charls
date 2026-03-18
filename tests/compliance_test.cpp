// SPDX-FileCopyrightText: © 2019 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>

#include <charls/charls.hpp>

#include "../test/portable_anymap_file.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <vector>

using namespace charls_test;
using std::array;
using std::byte;
using std::vector;

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

[[nodiscard]] vector<byte> read_file(const char* filename)
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

[[nodiscard]] portable_anymap_file read_anymap_reference_file(const char* filename,
                                                               const interleave_mode interleave_mode,
                                                               const frame_info& info)
{
    portable_anymap_file reference_file{filename};

    if (interleave_mode == interleave_mode::none && info.component_count == 3)
    {
        triplet_to_planar(reference_file.image_data(), info.width, info.height);
    }

    return reference_file;
}

[[nodiscard]] bool verify_encoded_bytes(const vector<byte>& uncompressed_source, const vector<byte>& encoded_source)
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
            return false;
    }

    return true;
}

void compare_buffers(const byte* buffer1, const size_t size1, const byte* buffer2, const size_t size2)
{
    ASSERT_EQ(size1, size2);

    for (size_t i{}; i != size1; ++i)
    {
        if (buffer1[i] != buffer2[i])
        {
            EXPECT_EQ(buffer1[i], buffer2[i]);
            break;
        }
    }
}

void test_compliance(const vector<byte>& encoded_source, const vector<byte>& uncompressed_source,
                     const bool check_encode)
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
            if (uncompressed_source[i] != destination[i])
            {
                ASSERT_EQ(uncompressed_source[i], destination[i]);
            }
        }
    }
    else
    {
        const frame_info fi{decoder.frame_info()};
        const auto near_lossless{decoder.get_near_lossless()};

        if (fi.bits_per_sample <= 8)
        {
            for (size_t i{}; i != uncompressed_source.size(); ++i)
            {
                if (std::abs(static_cast<int>(uncompressed_source[i]) - static_cast<int>(destination[i])) >
                    near_lossless)
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
                if (std::abs(static_cast<int>(source16[i]) - static_cast<int>(destination16[i])) > near_lossless)
                {
                    ASSERT_EQ(static_cast<int>(source16[i]), static_cast<int>(destination16[i]));
                }
            }
        }
    }
}

void decode_encode_file(const char* encoded_filename, const char* raw_filename, const bool check_encode = true)
{
    const auto encoded_source{read_file(encoded_filename)};
    const jpegls_decoder decoder{encoded_source, true};

    portable_anymap_file reference_file{
        read_anymap_reference_file(raw_filename, decoder.get_interleave_mode(), decoder.frame_info())};

    test_compliance(encoded_source, reference_file.image_data(), check_encode);
}

} // namespace


TEST(compliance_test, decode_encode_color_8_bit_interleave_none_lossless)
{
    // ISO 14495-1: official test image 1 (T87_test-1-2-3-4-5-6.zip)
    decode_encode_file("data/t8c0e0.jls", "data/test8.ppm");
}

TEST(compliance_test, decode_encode_color_8_bit_interleave_line_lossless)
{
    // ISO 14495-1: official test image 2 (T87_test-1-2-3-4-5-6.zip)
    decode_encode_file("data/t8c1e0.jls", "data/test8.ppm");
}

TEST(compliance_test, decode_encode_color_8_bit_interleave_sample_lossless)
{
    // ISO 14495-1: official test image 3 (T87_test-1-2-3-4-5-6.zip)
    decode_encode_file("data/t8c2e0.jls", "data/test8.ppm");
}

TEST(compliance_test, decode_encode_color_8_bit_interleave_none_near_lossless_3)
{
    // ISO 14495-1: official test image 4 (T87_test-1-2-3-4-5-6.zip)
    decode_encode_file("data/t8c0e3.jls", "data/test8.ppm");
}

TEST(compliance_test, decode_encode_color_8_bit_interleave_line_near_lossless_3)
{
    // ISO 14495-1: official test image 5 (T87_test-1-2-3-4-5-6.zip)
    decode_encode_file("data/t8c1e3.jls", "data/test8.ppm");
}

TEST(compliance_test, decode_encode_color_8_bit_interleave_sample_near_lossless_3)
{
    // ISO 14495-1: official test image 6 (T87_test-1-2-3-4-5-6.zip)
    decode_encode_file("data/t8c2e3.jls", "data/test8.ppm");
}

TEST(compliance_test, decode_encode_color_8_bit_interleave_none_lossless_non_default)
{
    // ISO 14495-1: official test image 9 (T87_test-1-2-3-4-5-6.zip)
    // NON-DEFAULT parameters T1=T2=T3=9,RESET=31.
    decode_encode_file("data/t8nde0.jls", "data/test8bs2.pgm");
}

TEST(compliance_test, decode_encode_color_8_bit_interleave_none_near_lossless_3_non_default)
{
    // ISO 14495-1: official test image 10 (T87_test-1-2-3-4-5-6.zip)
    // NON-DEFAULT parameters T1=T2=T3=9,RESET=31.
    decode_encode_file("data/t8nde3.jls", "data/test8bs2.pgm");
}

TEST(compliance_test, decode_encode_monochrome_16_bit_lossless)
{
    // ISO 14495-1: official test image 11 (T87_test-11-12.zip)
    // Note: test image is actually 12 bit.
    decode_encode_file("data/t16e0.jls", "data/test16.pgm");
}

TEST(compliance_test, decode_monochrome_16_bit_near_lossless_3)
{
    // ISO 14495-1: official test image 12 (T87_test-11-12.zip)
    // Note: test image is actually 12 bit.
    decode_encode_file("data/t16e3.jls", "data/test16.pgm", false);
}

TEST(compliance_test, decode_encode_tulips_monochrome_8_bit_lossless_hp)
{
    // Additional, Tulips encoded with HP 1.0BETA encoder.
    decode_encode_file("data/tulips-gray-8bit-512-512-hp-encoder.jls", "data/tulips-gray-8bit-512-512.pgm");
}

TEST(compliance_test, decode_color_8_bit_interleave_none_lossless_restart_7)
{
    // ISO 14495-1: official test image 1 but with restart markers.
    decode_encode_file("data/test8_ilv_none_rm_7.jls", "data/test8.ppm", false);
}

TEST(compliance_test, decode_color_8_bit_interleave_line_lossless_restart_7)
{
    // ISO 14495-1: official test image 2 but with restart markers.
    decode_encode_file("data/test8_ilv_line_rm_7.jls", "data/test8.ppm", false);
}

TEST(compliance_test, decode_color_8_bit_interleave_sample_lossless_restart_7)
{
    // ISO 14495-1: official test image 3 but with restart markers.
    decode_encode_file("data/test8_ilv_sample_rm_7.jls", "data/test8.ppm", false);
}

TEST(compliance_test, decode_color_8_bit_interleave_sample_lossless_restart_300)
{
    // ISO 14495-1: official test image 3 but with restart markers and restart interval 300
    decode_encode_file("data/test8_ilv_sample_rm_300.jls", "data/test8.ppm", false);
}

TEST(compliance_test, decode_monochrome_16_bit_restart_5)
{
    // ISO 14495-1: official test image 12 but with restart markers and restart interval 5
    decode_encode_file("data/test16_rm_5.jls", "data/test16.pgm", false);
}

TEST(compliance_test, decode_mapping_table_sample_annex_h4_5)
{
    // ISO 14495-1: Sample image from appendix H.4.5 "Example of a palletised image" / Figure H.10
    constexpr array palletised_data{
        byte{0xFF}, byte{0xD8}, // Start of image (SOI) marker
        byte{0xFF}, byte{0xF7}, // Start of JPEG-LS frame (SOF 55) marker - marker segment follows
        byte{0x00}, byte{0x0B}, // Length of marker segment = 11 bytes including the length field
        byte{0x02},             // P = Precision = 2 bits per sample
        byte{0x00}, byte{0x04}, // Y = Number of lines = 4
        byte{0x00}, byte{0x03}, // X = Number of columns = 3
        byte{0x01},             // Nf = Number of components in the frame = 1
        byte{0x01},             // C1  = Component ID = 1 (first and only component)
        byte{0x11},             // Sub-sampling: H1 = 1, V1 = 1
        byte{0x00},             // Tq1 = 0 (this field is always 0)

        byte{0xFF}, byte{0xF8},             // LSE - JPEG-LS preset parameters marker
        byte{0x00}, byte{0x11},             // Length of marker segment = 17 bytes including the length field
        byte{0x02},                         // ID = 2, mapping table
        byte{0x05},                         // TID = 5 Table identifier (arbitrary)
        byte{0x03},                         // Wt = 3 Width of table entry
        byte{0xFF}, byte{0xFF}, byte{0xFF}, // Entry for index 0
        byte{0xFF}, byte{0x00}, byte{0x00}, // Entry for index 1
        byte{0x00}, byte{0xFF}, byte{0x00}, // Entry for index 2
        byte{0x00}, byte{0x00}, byte{0xFF}, // Entry for index 3

        byte{0xFF}, byte{0xDA},             // Start of scan (SOS) marker
        byte{0x00}, byte{0x08},             // Length of marker segment = 8 bytes including the length field
        byte{0x01},                         // Ns = Number of components for this scan = 1
        byte{0x01},                         // C1 = Component ID = 1
        byte{0x05},                         // Tm 1  = Mapping table identifier = 5
        byte{0x00},                         // NEAR = 0 (near-lossless max error)
        byte{0x00},                         // ILV = 0 (interleave mode = non-interleaved)
        byte{0x00},                         // Al = 0, Ah = 0 (no point transform)
        byte{0xDB}, byte{0x95}, byte{0xF0}, // 3 bytes of compressed image data
        byte{0xFF}, byte{0xD9}              // End of image (EOI) marker
    };

    jpegls_decoder decoder;
    decoder.source(palletised_data);
    decoder.read_header();
    vector<byte> destination(decoder.get_destination_size());
    decoder.decode(destination);

    constexpr array expected{byte{0}, byte{0}, byte{1}, byte{1}, byte{1}, byte{2},
                             byte{2}, byte{2}, byte{3}, byte{3}, byte{3}, byte{3}};
    compare_buffers(expected.data(), expected.size(), destination.data(), destination.size());

    const int32_t mapping_table_id{decoder.get_mapping_table_id(0)};
    EXPECT_EQ(5, mapping_table_id);

    const auto table_index{decoder.find_mapping_table_index(mapping_table_id)};

    const mapping_table_info table_info{decoder.get_mapping_table_info(table_index)};
    vector<byte> mapping_table(table_info.data_size);

    decoder.get_mapping_table_data(table_index, mapping_table);

    constexpr array expected_mapping_table{byte{0xFF}, byte{0xFF}, byte{0xFF}, byte{0xFF}, byte{0}, byte{0},
                                           byte{0},    byte{0xFF}, byte{0},    byte{0},    byte{0}, byte{0xFF}};
    compare_buffers(expected_mapping_table.data(), expected_mapping_table.size(), mapping_table.data(),
                    mapping_table.size());
}

} // namespace charls::test
