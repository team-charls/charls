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
        encode("DataFiles/TEST16.pgm", interleave_mode::none);
    }

    TEST_METHOD(encode_monochrome_16_bit_lossless)
    {
        encode("DataFiles/16-bit-640-480-many-dots.pgm", interleave_mode::none);
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

        encode(reference_file, interleave_mode);
        encode_legacy_api(reference_file, interleave_mode);
    }

    static void encode(const portable_anymap_file& reference_file, const interleave_mode interleave_mode)
    {
        jpegls_encoder encoder;
        encoder.frame_info({
            static_cast<uint32_t>(reference_file.width()), static_cast<uint32_t>(reference_file.height()),
            reference_file.bits_per_sample(), reference_file.component_count()})
            .interleave_mode(interleave_mode);

        vector<uint8_t> charls_encoded(encoder.estimated_destination_size());
        encoder.destination(charls_encoded);

        const size_t bytes_written = encoder.encode(reference_file.image_data());
        charls_encoded.resize(bytes_written);

        test_by_decoding(charls_encoded, reference_file, interleave_mode);
    }

    static void encode_legacy_api(const portable_anymap_file& reference_file, const interleave_mode interleave_mode)
    {
        JlsParameters info{};
        info.width = reference_file.width();
        info.height = reference_file.height();
        info.bitsPerSample = reference_file.bits_per_sample();
        info.components = reference_file.component_count();
        info.interleaveMode = interleave_mode;

        vector<uint8_t> charls_encoded(estimated_destination_size(reference_file.width(), reference_file.height(),
            reference_file.component_count(), reference_file.bits_per_sample()));

        size_t bytesWritten;
        const auto error = JpegLsEncode(charls_encoded.data(), charls_encoded.size(), &bytesWritten,
            reference_file.image_data().data(), reference_file.image_data().size(), &info, nullptr);
        Assert::AreEqual(jpegls_errc::success, error);

        test_by_decoding(charls_encoded, reference_file, interleave_mode);
    }

    static void test_by_decoding(const vector<uint8_t>& encoded_source, const portable_anymap_file& reference_file, const interleave_mode interleave_mode)
    {
        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        const auto frame_info = decoder.frame_info();
        Assert::AreEqual(static_cast<uint32_t>(reference_file.width()), frame_info.width);
        Assert::AreEqual(static_cast<uint32_t>(reference_file.height()), frame_info.height);
        Assert::AreEqual(reference_file.bits_per_sample(), frame_info.bits_per_sample);
        Assert::AreEqual(reference_file.component_count(), frame_info.component_count);
        Assert::IsTrue(interleave_mode == decoder.interleave_mode());

        vector<uint8_t> destination(decoder.destination_size());
        decoder.decode(destination);

        const vector<uint8_t>& uncompressed_source = reference_file.image_data();

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

    constexpr static size_t estimated_destination_size(int width, int height, int component_count, int bits_per_sample) noexcept
    {
        return static_cast<size_t>(width) * height *
                   component_count * (bits_per_sample < 9 ? 1 : 2) + 1024;
    }
};

} // namespace CharLSUnitTest
