// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "compliance.h"
#include "util.h"

#include <array>
#include <cstring>
#include <iostream>
#include <vector>

using std::array;
using std::cout;
using std::swap;
using std::vector;
using namespace charls;

namespace {

void triplet2_planar(vector<uint8_t>& buffer, const rect_size size)
{
    vector<uint8_t> work_buffer(buffer.size());

    const size_t byte_count = size.cx * size.cy;
    for (size_t index = 0; index < byte_count; ++index)
    {
        work_buffer[index] = buffer[index * 3 + 0];
        work_buffer[index + 1 * byte_count] = buffer[index * 3 + 1];
        work_buffer[index + 2 * byte_count] = buffer[index * 3 + 2];
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
        encoder.interleave_mode(decoder.interleave_mode());
        encoder.near_lossless(decoder.near_lossless());
        encoder.preset_coding_parameters(decoder.preset_coding_parameters());
        static_cast<void>(encoder.encode(uncompressed_data, uncompressed_length));

        for (size_t i = 0; i < compressed_length; ++i)
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


void test_compliance(const uint8_t* compressed_bytes, const size_t compressed_length, const uint8_t* uncompressed_data,
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

        const auto destination{decoder.decode<vector<uint8_t>>()};

        if (decoder.near_lossless() == 0)
        {
            for (size_t i = 0; i < uncompressed_length; ++i)
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
    const vector<uint8_t> encoded_buffer = read_file(name_encoded);

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

    vector<uint8_t> raw_buffer = read_file(name_raw, offset);

    const auto& frame_info = decoder.frame_info();
    if (frame_info.bits_per_sample > 8)
    {
        fix_endian(&raw_buffer, false);
    }

    if (decoder.interleave_mode() == interleave_mode::none && frame_info.component_count == 3)
    {
        triplet2_planar(raw_buffer, rect_size(frame_info.width, frame_info.height));
    }

    test_compliance(encoded_buffer.data(), encoded_buffer.size(), raw_buffer.data(), raw_buffer.size(), check_encode);
}


////uint8_t palettisedDataH10[] = {
////    0xFF, 0xD8, //Start of image (SOI) marker
////    0xFF, 0xF7, //Start of JPEG-LS frame (SOF 55) marker – marker segment follows
////    0x00, 0x0B, //Length of marker segment = 11 bytes including the length field
////    0x02, //P = Precision = 2 bits per sample
////    0x00, 0x04, //Y = Number of lines = 4
////    0x00, 0x03, //X = Number of columns = 3
////    0x01, //Nf = Number of components in the frame = 1
////    0x01, //C1  = Component ID = 1 (first and only component)
////    0x11, //Sub-sampling: H1 = 1, V1 = 1
////    0x00, //Tq1 = 0 (this field is always 0)
////
////    0xFF, 0xF8, //LSE – JPEG-LS preset parameters marker
////    0x00, 0x11, //Length of marker segment = 17 bytes including the length field
////    0x02, //ID = 2, mapping table
////    0x05, //TID = 5 Table identifier (arbitrary)
////    0x03, //Wt = 3 Width of table entry
////    0xFF, 0xFF, 0xFF, //Entry for index 0
////    0xFF, 0x00, 0x00, //Entry for index 1
////    0x00, 0xFF, 0x00, //Entry for index 2
////    0x00, 0x00, 0xFF, //Entry for index 3
////
////    0xFF, 0xDA, //Start of scan (SOS) marker
////    0x00, 0x08, //Length of marker segment = 8 bytes including the length field
////    0x01, //Ns = Number of components for this scan = 1
////    0x01, //C1 = Component ID = 1
////    0x05, //Tm 1  = Mapping table identifier = 5
////    0x00, //NEAR = 0 (near-lossless max error)
////    0x00, //ILV = 0 (interleave mode = non-interleaved)
////    0x00, //Al = 0, Ah = 0 (no point transform)
////    0xDB, 0x95, 0xF0, //3 bytes of compressed image data
////    0xFF, 0xD9 //End of image (EOI) marker
////};


const array<uint8_t, 16> buffer = {0, 0, 90, 74, 68, 50, 43, 205, 64, 145, 145, 145, 100, 145, 145, 145};
////const uint8_t bufferEncoded[] =   {   0xFF, 0xD8, 0xFF, 0xF7, 0x00, 0x0B, 0x08, 0x00, 0x04, 0x00, 0x04, 0x01, 0x01, 0x11,
///0x00, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, /0xC0, 0x00, 0x00, 0x6C, 0x80, 0x20, 0x8E, /0x01, 0xC0,
///0x00, 0x00, 0x57, 0x40, 0x00, 0x00, 0x6E, 0xE6, 0x00, 0x00, 0x01, 0xBC, 0x18, 0x00, /0x00, 0x05, 0xD8, 0x00, 0x00, 0x91,
///0x60, 0xFF, 0xD9};

} // namespace


void test_sample_annex_h3()
{
    ////rect_size size = rect_size(4,4);
    vector<uint8_t> vec_raw(16);
    memcpy(vec_raw.data(), buffer.data(), buffer.size());
    ////  TestJls(vecRaw, size, 8, 1, ILV_NONE, bufferEncoded, sizeof(bufferEncoded), false);
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

    // additional, Lena compressed with other codec (UBC?), vfy with CharLS
    decompress_file("test/lena8b.jls", "test/lena8b.raw", 0);
}
