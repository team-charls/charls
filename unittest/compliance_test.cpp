// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "util.hpp"

#include <charls/charls.hpp>

#include "../test/portable_anymap_file.hpp"

using namespace charls_test;
using std::byte;
using std::array;
using std::vector;
using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace {

void compare_buffers(const byte* buffer1, const size_t size1, const byte* buffer2, const size_t size2)
{
    Assert::AreEqual(size1, size2);

    for (size_t i{}; i != size1; ++i)
    {
        if (buffer1[i] != buffer2[i])
        {
            Assert::AreEqual(buffer1[i], buffer2[i]);
            break;
        }
    }
}

} // namespace

namespace charls::test {

TEST_CLASS(compliance_test)
{
public:
    TEST_METHOD(decode_encode_color_8_bit_interleave_none_lossless) // NOLINT
    {
        // ISO 14495-1: official test image 1 (T87_test-1-2-3-4-5-6.zip)
        decode_encode_file("DataFiles/t8c0e0.jls", "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_encode_color_8_bit_interleave_line_lossless) // NOLINT
    {
        // ISO 14495-1: official test image 2 (T87_test-1-2-3-4-5-6.zip)
        decode_encode_file("DataFiles/t8c1e0.jls", "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_encode_color_8_bit_interleave_sample_lossless) // NOLINT
    {
        // ISO 14495-1: official test image 3 (T87_test-1-2-3-4-5-6.zip)
        decode_encode_file("DataFiles/t8c2e0.jls", "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_encode_color_8_bit_interleave_none_near_lossless_3) // NOLINT
    {
        // ISO 14495-1: official test image 4 (T87_test-1-2-3-4-5-6.zip)
        decode_encode_file("DataFiles/t8c0e3.jls", "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_encode_color_8_bit_interleave_line_near_lossless_3) // NOLINT
    {
        // ISO 14495-1: official test image 5 (T87_test-1-2-3-4-5-6.zip)
        decode_encode_file("DataFiles/t8c1e3.jls", "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_encode_color_8_bit_interleave_sample_near_lossless_3) // NOLINT
    {
        // ISO 14495-1: official test image 6 (T87_test-1-2-3-4-5-6.zip)
        decode_encode_file("DataFiles/t8c2e3.jls", "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_encode_color_8_bit_interleave_none_lossless_non_default) // NOLINT
    {
        // ISO 14495-1: official test image 9 (T87_test-1-2-3-4-5-6.zip)
        // NON-DEFAULT parameters T1=T2=T3=9,RESET=31.
        decode_encode_file("DataFiles/t8nde0.jls", "DataFiles/test8bs2.pgm");
    }

    TEST_METHOD(decode_encode_color_8_bit_interleave_none_near_lossless_3_non_default) // NOLINT
    {
        // ISO 14495-1: official test image 10 (T87_test-1-2-3-4-5-6.zip)
        // NON-DEFAULT parameters T1=T2=T3=9,RESET=31.
        decode_encode_file("DataFiles/t8nde3.jls", "DataFiles/test8bs2.pgm");
    }

    TEST_METHOD(decode_encode_monochrome_16_bit_lossless) // NOLINT
    {
        // ISO 14495-1: official test image 11 (T87_test-11-12.zip)
        // Note: test image is actually 12 bit.
        decode_encode_file("DataFiles/t16e0.jls", "DataFiles/test16.pgm");
    }

    TEST_METHOD(decode_monochrome_16_bit_near_lossless_3) // NOLINT
    {
        // ISO 14495-1: official test image 12 (T87_test-11-12.zip)
        // Note: test image is actually 12 bit.
        decode_encode_file("DataFiles/t16e3.jls", "DataFiles/TEST16.pgm", false);
    }

    TEST_METHOD(decode_encode_tulips_monochrome_8_bit_lossless_hp) // NOLINT
    {
        // Additional, Tulips encoded with HP 1.0BETA encoder.
        decode_encode_file("DataFiles/tulips-gray-8bit-512-512-hp-encoder.jls", "DataFiles/tulips-gray-8bit-512-512.pgm");
    }

    TEST_METHOD(decode_color_8_bit_interleave_none_lossless_restart_7) // NOLINT
    {
        // ISO 14495-1: official test image 1 but with restart markers.
        decode_encode_file("DataFiles/test8_ilv_none_rm_7.jls", "DataFiles/test8.ppm", false);
    }

    TEST_METHOD(decode_color_8_bit_interleave_line_lossless_restart_7) // NOLINT
    {
        // ISO 14495-1: official test image 2 but with restart markers.
        decode_encode_file("DataFiles/test8_ilv_line_rm_7.jls", "DataFiles/test8.ppm", false);
    }

    TEST_METHOD(decode_color_8_bit_interleave_sample_lossless_restart_7) // NOLINT
    {
        // ISO 14495-1: official test image 3 but with restart markers.
        decode_encode_file("DataFiles/test8_ilv_sample_rm_7.jls", "DataFiles/test8.ppm", false);
    }

    TEST_METHOD(decode_color_8_bit_interleave_sample_lossless_restart_300) // NOLINT
    {
        // ISO 14495-1: official test image 3 but with restart markers and restart interval 300
        decode_encode_file("DataFiles/test8_ilv_sample_rm_300.jls", "DataFiles/test8.ppm", false);
    }

    TEST_METHOD(decode_monochrome_16_bit_restart_5) // NOLINT
    {
        // ISO 14495-1: official test image 12 but with restart markers and restart interval 5
        decode_encode_file("DataFiles/test16_rm_5.jls", "DataFiles/test16.pgm", false);
    }

    TEST_METHOD(decode_mapping_table_sample_annex_h4_5) // NOLINT
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
        Assert::AreEqual(5, mapping_table_id);

        const auto table_index{decoder.find_mapping_table_index(mapping_table_id)};

        const mapping_table_info table_info{decoder.get_mapping_table_info(table_index)};
        vector<byte> mapping_table(table_info.data_size);

        decoder.get_mapping_table_data(table_index, mapping_table);

        constexpr array expected_mapping_table{byte{0xFF}, byte{0xFF}, byte{0xFF}, byte{0xFF}, byte{0}, byte{0},
                                               byte{0},    byte{0xFF}, byte{0},    byte{0},    byte{0}, byte{0xFF}};
        compare_buffers(expected_mapping_table.data(), expected_mapping_table.size(), mapping_table.data(),
                        mapping_table.size());
    }

private:
    static void decode_encode_file(const char* encoded_filename, const char* raw_filename, const bool check_encode = true)
    {
        const auto encoded_source{read_file(encoded_filename)};
        const jpegls_decoder decoder{encoded_source, true};

        portable_anymap_file reference_file{
            read_anymap_reference_file(raw_filename, decoder.get_interleave_mode(), decoder.frame_info())};

        test_compliance(encoded_source, reference_file.image_data(), check_encode);
    }
};

} // namespace charls::test
