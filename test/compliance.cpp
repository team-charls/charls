// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "compliance.hpp"

#include "util.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <tuple>
#include <vector>

using std::array;
using std::byte;
using std::cout;
using std::swap;
using std::vector;
using namespace charls;

namespace {

void compare_buffers(const byte* data1, const size_t size1, const byte* data2, const size_t size2)
{
    assert::is_true(size1 == size2);

    for (size_t i{}; i != size1; ++i)
    {
        if (data1[i] != data2[i])
        {
            assert::is_true(false);
            break;
        }
    }
}

void triplet2_planar(vector<byte>& buffer, const rect_size size)
{
    vector<byte> work_buffer(buffer.size());

    const size_t byte_count{size.cx * size.cy};
    for (size_t i{}; i != byte_count; ++i)
    {
        work_buffer[i] = buffer[i * 3];
        work_buffer[i + (1 * byte_count)] = buffer[(i * 3) + 1];
        work_buffer[i + (2 * byte_count)] = buffer[(i * 3) + 2];
    }
    swap(buffer, work_buffer);
}


bool verify_encoded_bytes(const void* uncompressed_data, const size_t uncompressed_length, const void* compressed_data,
                          const size_t compressed_length)
{
    try
    {
        jpegls_decoder decoder;
        decoder.source(compressed_data, compressed_length).read_header();

        vector<uint8_t> our_encoded_bytes(compressed_length + 16);

        jpegls_encoder encoder;
        encoder.destination(our_encoded_bytes);
        encoder.frame_info(decoder.frame_info());
        encoder.interleave_mode(decoder.get_interleave_mode());
        encoder.near_lossless(decoder.get_near_lossless());
        encoder.preset_coding_parameters(decoder.preset_coding_parameters());
        std::ignore = encoder.encode(uncompressed_data, uncompressed_length);

        for (size_t i{}; i != compressed_length; ++i)
        {
            if (static_cast<const uint8_t*>(compressed_data)[i] != our_encoded_bytes[i])
            {
                return false;
            }
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}


void test_compliance(const byte* compressed_bytes, const size_t compressed_length, const byte* uncompressed_data,
                     const size_t uncompressed_length, const bool check_encode)
{
    try
    {
        jpegls_decoder decoder;
        decoder.source(compressed_bytes, compressed_length).read_header();

        if (check_encode)
        {
            assert::is_true(
                verify_encoded_bytes(uncompressed_data, uncompressed_length, compressed_bytes, compressed_length));
        }

        const auto destination{decoder.decode<vector<byte>>()};

        if (decoder.get_near_lossless() == 0)
        {
            for (size_t i{}; i != uncompressed_length; ++i)
            {
                if (uncompressed_data[i] != destination[i])
                {
                    assert::is_true(false);
                    break;
                }
            }
        }
    }
    catch (const jpegls_error&)
    {
        assert::is_true(false);
    }
}


void decompress_file(const char* name_encoded, const char* name_raw, const int offset, const bool check_encode = true)
{
    cout << "Conformance test:" << name_encoded << "\n\r";
    const auto encoded_buffer{read_file(name_encoded)};

    jpegls_decoder decoder;
    try
    {
        decoder.source(encoded_buffer).read_header();
    }
    catch (...)
    {
        assert::is_true(false);
        return;
    }

    auto raw_buffer{read_file(name_raw, offset)};

    const auto& [width, height, bits_per_sample, component_count]{decoder.frame_info()};
    if (bits_per_sample > 8)
    {
        fix_endian(&raw_buffer, false);
    }

    if (decoder.get_interleave_mode() == interleave_mode::none && component_count == 3)
    {
        triplet2_planar(raw_buffer, {width, height});
    }

    test_compliance(encoded_buffer.data(), encoded_buffer.size(), raw_buffer.data(), raw_buffer.size(), check_encode);
}


constexpr array<uint8_t, 16> buffer{0, 0, 90, 74, 68, 50, 43, 205, 64, 145, 145, 145, 100, 145, 145, 145};
////const uint8_t bufferEncoded[] =   {   0xFF, 0xD8, 0xFF, 0xF7, 0x00, 0x0B, 0x08, 0x00, 0x04, 0x00, 0x04, 0x01, 0x01, 0x11,
/// 0x00, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, /0xC0, 0x00, 0x00, 0x6C, 0x80, 0x20, 0x8E, /0x01, 0xC0,
/// 0x00, 0x00, 0x57, 0x40, 0x00, 0x00, 0x6E, 0xE6, 0x00, 0x00, 0x01, 0xBC, 0x18, 0x00, /0x00, 0x05, 0xD8, 0x00, 0x00, 0x91,
/// 0x60, 0xFF, 0xD9};

} // namespace


void test_sample_annex_h3()
{
    ////rect_size size = rect_size(4,4);
    vector<uint8_t> vec_raw(16);
    memcpy(vec_raw.data(), buffer.data(), buffer.size());
    ////  TestJls(vecRaw, size, 8, 1, ILV_NONE, bufferEncoded, sizeof(bufferEncoded), false);
}


void test_sample_annex_h4_5()
{
    constexpr array palettised_data{
        byte{0xFF}, byte{0xD8}, // Start of image (SOI) marker
        byte{0xFF}, byte{0xF7}, // Start of JPEG-LS frame (SOF 55) marker – marker segment follows
        byte{0x00}, byte{0x0B}, // Length of marker segment = 11 bytes including the length field
        byte{0x02},             // P = Precision = 2 bits per sample
        byte{0x00}, byte{0x04}, // Y = Number of lines = 4
        byte{0x00}, byte{0x03}, // X = Number of columns = 3
        byte{0x01},             // Nf = Number of components in the frame = 1
        byte{0x01},             // C1  = Component ID = 1 (first and only component)
        byte{0x11},             // Sub-sampling: H1 = 1, V1 = 1
        byte{0x00},             // Tq1 = 0 (this field is always 0)

        byte{0xFF}, byte{0xF8},             // LSE – JPEG-LS preset parameters marker
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
    decoder.source(palettised_data);
    decoder.read_header();
    vector<byte> destination(decoder.get_destination_size());
    decoder.decode(destination);

    constexpr array expected{byte{0}, byte{0}, byte{1}, byte{1}, byte{1}, byte{2},
                             byte{2}, byte{2}, byte{3}, byte{3}, byte{3}, byte{3}};
    compare_buffers(expected.data(), expected.size(), destination.data(), destination.size());

    const int32_t mapping_table_id{decoder.get_mapping_table_id(0)};
    assert::is_true(mapping_table_id == 5);

    const auto table_index{decoder.find_mapping_table_index(mapping_table_id)};

    const mapping_table_info table_info{decoder.get_mapping_table_info(table_index)};
    vector<byte> mapping_table(table_info.data_size);

    decoder.get_mapping_table_data(table_index, mapping_table);

    constexpr array expected_mapping_table{byte{0xFF}, byte{0xFF}, byte{0xFF}, byte{0xFF}, byte{0}, byte{0},
                                           byte{0},    byte{0xFF}, byte{0},    byte{0},    byte{0}, byte{0xFF}};
    compare_buffers(expected_mapping_table.data(), expected_mapping_table.size(), mapping_table.data(),
                    mapping_table.size());
}


void test_color_transforms_hp_images()
{
    decompress_file("test/jlsimage/banny_normal.jls", "test/jlsimage/banny.ppm", 38, false);
    decompress_file("test/jlsimage/banny_HP1.jls", "test/jlsimage/banny.ppm", 38, false);
    decompress_file("test/jlsimage/banny_HP2.jls", "test/jlsimage/banny.ppm", 38, false);
    decompress_file("test/jlsimage/banny_HP3.jls", "test/jlsimage/banny.ppm", 38, false);
}


void test_conformance()
{
    // Test 1
    decompress_file("test/conformance/t8c0e0.jls", "test/conformance/test8.ppm", 15);

    // Test 2
    decompress_file("test/conformance/t8c1e0.jls", "test/conformance/test8.ppm", 15);

    // Test 3
    decompress_file("test/conformance/t8c2e0.jls", "test/conformance/test8.ppm", 15);

    // Test 4
    decompress_file("test/conformance/t8c0e3.jls", "test/conformance/test8.ppm", 15);

    // Test 5
    decompress_file("test/conformance/t8c1e3.jls", "test/conformance/test8.ppm", 15);

    // Test 6
    decompress_file("test/conformance/t8c2e3.jls", "test/conformance/test8.ppm", 15);

    // Test 7
    // Test 8

    // Test 9
    decompress_file("test/conformance/t8nde0.jls", "test/conformance/test8bs2.pgm", 15);

    // Test 10
    decompress_file("test/conformance/t8nde3.jls", "test/conformance/test8bs2.pgm", 15);

    // Test 11
    decompress_file("test/conformance/t16e0.jls", "test/conformance/test16.pgm", 16);

    // Test 12
    decompress_file("test/conformance/t16e3.jls", "test/conformance/test16.pgm", 16);
}
