// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include "../test/portable_anymap_file.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using namespace charls;
using namespace charls_test;
using std::vector;

// clang-format off

namespace CharLSUnitTest {

TEST_CLASS(encode_test)
{
public:
    TEST_METHOD(encode_monochrome_12_bit_lossless)
    {
        const portable_anymap_file reference_file = read_anymap_reference_file("DataFiles/TEST16.pgm", interleave_mode::none);

        jpegls_encoder encoder;
        encoder.frame_info({
            static_cast<uint32_t>(reference_file.width()), static_cast<uint32_t>(reference_file.height()),
            reference_file.bits_per_sample(), reference_file.component_count()});

        vector<uint8_t> charls_encoded_data(encoder.estimated_destination_size());
        encoder.destination(charls_encoded_data);

        const size_t bytes_written = encoder.encode(reference_file.image_data());
        charls_encoded_data.resize(bytes_written);

        test_by_decoding(charls_encoded_data, reference_file.image_data());
    }

    TEST_METHOD(encode_color_8_bit_interleave_none_lossless)
    {
        encode("DataFiles/TEST8.PPM", interleave_mode::none);
    }

    TEST_METHOD(encode_color_8_bit_interleave_line_lossless)
    {
        encode("DataFiles/TEST8.PPM", interleave_mode::line);
    }

    TEST_METHOD(encode_color_8_bit_interleave_sample_lossless)
    {
        encode("DataFiles/TEST8.PPM", interleave_mode::sample);
    }

private:
    static void encode(const char* filename, const interleave_mode interleave_mode)
    {
        const portable_anymap_file reference_file = read_anymap_reference_file(filename, interleave_mode);

        jpegls_encoder encoder;
        encoder.frame_info({
            static_cast<uint32_t>(reference_file.width()), static_cast<uint32_t>(reference_file.height()),
            reference_file.bits_per_sample(), reference_file.component_count()});

        vector<uint8_t> charls_encoded(encoder.estimated_destination_size());
        encoder.destination(charls_encoded);

        const size_t bytes_written = encoder.encode(reference_file.image_data());
        charls_encoded.resize(bytes_written);

        test_by_decoding(charls_encoded, reference_file.image_data());
    }

    static void test_by_decoding(const vector<uint8_t>& encoded_source, const vector<uint8_t>& uncompressed_source)
    {
        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        vector<uint8_t> destination(decoder.destination_size());
        decoder.decode(destination);

        Assert::AreEqual(destination.size(), uncompressed_source.size());

        if (decoder.near_lossless() == 0)
        {
            for (size_t i = 0; i < uncompressed_source.size(); ++i)
            {
                if (destination[i] != uncompressed_source[i]) // AreEqual is very slow, pre-test to speed up 50X
                {
                    Assert::AreEqual(destination[i], uncompressed_source[i]);
                }
            }
        }
    }
};

} // namespace CharLSUnitTest
