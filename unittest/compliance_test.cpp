// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include "../test/portable_anymap_file.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using namespace charls;
using namespace charls_test;
using std::error_code;
using std::vector;


// clang-format off

namespace CharLSUnitTest {

TEST_CLASS(compliance_test)
{
public:
    TEST_METHOD(decompress_color_8_bit_interleave_none_lossless)
    {
        // ISO 14495-1: official test image 1 (T87_test-1-2-3-4-5-6.zip)
        decompress_file("DataFiles/T8C0E0.JLS", "DataFiles/TEST8.PPM");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_line_lossless)
    {
        // ISO 14495-1: official test image 2 (T87_test-1-2-3-4-5-6.zip)
        decompress_file("DataFiles/T8C1E0.JLS", "DataFiles/TEST8.PPM");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_sample_lossless)
    {
        // ISO 14495-1: official test image 3 (T87_test-1-2-3-4-5-6.zip)
        decompress_file("DataFiles/T8C2E0.JLS", "DataFiles/TEST8.PPM");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_none_near_lossless_3)
    {
        // ISO 14495-1: official test image 4 (T87_test-1-2-3-4-5-6.zip)
        decompress_file("DataFiles/T8C2E3.JLS", "DataFiles/TEST8.PPM");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_line_near_lossless_3)
    {
        // ISO 14495-1: official test image 5 (T87_test-1-2-3-4-5-6.zip)
        decompress_file("DataFiles/T8C1E3.JLS", "DataFiles/TEST8.PPM");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_sample_near_lossless_3)
    {
        // ISO 14495-1: official test image 6 (T87_test-1-2-3-4-5-6.zip)
        decompress_file("DataFiles/T8C2E3.JLS", "DataFiles/TEST8.PPM");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_line_lossless_non_default)
    {
        // ISO 14495-1: official test image 9 (T87_test-1-2-3-4-5-6.zip)
        // NON-DEFAULT parameters T1=T2=T3=9,RESET=31.
        decompress_file("DataFiles/T8NDE0.JLS", "DataFiles/TEST8BS2.PGM");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_line_near_lossless_3_non_default)
    {
        // ISO 14495-1: official test image 10 (T87_test-1-2-3-4-5-6.zip)
        // NON-DEFAULT parameters T1=T2=T3=9,RESET=31.
        decompress_file("DataFiles/T8NDE3.JLS", "DataFiles/TEST8BS2.PGM");
    }

    TEST_METHOD(decompress_monochrome_16_bit_lossless)
    {
        // ISO 14495-1: official test image 11 (T87_test-11-12.zip)
        // Note: test image is actually 12 bit.
        decompress_file("DataFiles/T16E0.JLS", "DataFiles/TEST16.PGM");
    }

    TEST_METHOD(decompress_monochrome_16_bit_near_lossless_3)
    {
        // ISO 14495-1: official test image 12 (T87_test-11-12.zip)
        // Note: test image is actually 12 bit.
        decompress_file("DataFiles/T16E3.JLS", "DataFiles/TEST16.pgm", false);
    }

    TEST_METHOD(lena_monochrome_8_bit_lossless_ubc)
    {
        // Additional, Lena compressed with other codec (UBC?)
        decompress_file("DataFiles/lena8b.jls", "DataFiles/lena8b.pgm");
    }

private:
    static void decompress_file(const char* encoded_filename, const char* raw_filename, bool check_encode = true)
    {
        vector<uint8_t> encoded_source = read_file(encoded_filename);

        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        portable_anymap_file reference_file = read_anymap_reference_file(raw_filename, decoder.interleave_mode(), decoder.frame_info());

        test_compliance(encoded_source, reference_file.image_data(), check_encode);
        test_compliance_legacy_api(encoded_source.data(), encoded_source.size(), reference_file.image_data().data(), reference_file.image_data().size(), check_encode);
    }

    static void test_compliance(const vector<uint8_t>& encoded_source, const vector<uint8_t>& uncompressed_source, bool checkEncode)
    {
        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        if (checkEncode)
        {
            Assert::IsTrue(verify_encoded_bytes(uncompressed_source, encoded_source));
        }

        vector<uint8_t> destination(decoder.destination_size());
        decoder.decode(destination);

        if (decoder.near_lossless() == 0)
        {
            for (size_t i = 0; i < uncompressed_source.size(); ++i)
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
                for (size_t i = 0; i < uncompressed_source.size(); ++i)
                {
                    if (abs(uncompressed_source[i] - destination[i]) > near_lossless) // AreEqual is very slow, pre-test to speed up 50X
                    {
                        Assert::AreEqual(uncompressed_source[i], destination[i]);
                    }
                }
            }
            else
            {
                const auto* source16 = reinterpret_cast<const uint16_t*>(uncompressed_source.data());
                const auto* destination16 = reinterpret_cast<const uint16_t*>(destination.data());

                for (size_t i = 0; i < uncompressed_source.size() / 2; ++i)
                {
                    if (abs(source16[i] - destination16[i]) > near_lossless) // AreEqual is very slow, pre-test to speed up 50X
                    {
                        Assert::AreEqual(static_cast<int>(source16[i]), static_cast<int>(destination16[i]));
                    }
                }
            }
        }
    }

    static void test_compliance_legacy_api(const uint8_t* compressedBytes, size_t compressedLength, const uint8_t* uncompressedData, size_t uncompressedLength, bool checkEncode)
    {
        JlsParameters info{};
        error_code error = JpegLsReadHeader(compressedBytes, compressedLength, &info, nullptr);
        Assert::IsFalse(static_cast<bool>(error));

        if (checkEncode)
        {
            Assert::IsTrue(verify_encoded_bytes_legacy_api(uncompressedData, uncompressedLength, compressedBytes, compressedLength));
        }

        vector<uint8_t> destination(static_cast<size_t>(info.height) *info.width * ((info.bitsPerSample + 7) / 8) * info.components);

        error = JpegLsDecode(destination.data(), destination.size(), compressedBytes, compressedLength, nullptr, nullptr);
        Assert::IsTrue(!error);

        if (info.allowedLossyError == 0)
        {
            for (size_t i = 0; i < uncompressedLength; ++i)
            {
                if (uncompressedData[i] != destination[i]) // AreEqual is very slow, pre-test to speed up 50X
                {
                    Assert::AreEqual(uncompressedData[i], destination[i]);
                }
            }
        }
    }

    static bool verify_encoded_bytes_legacy_api(const void* uncompressedData, size_t uncompressedLength, const void* compressedData, size_t compressedLength)
    {
        JlsParameters info{};
        error_code error = JpegLsReadHeader(compressedData, compressedLength, &info, nullptr);
        if (error)
            return false;

        vector<uint8_t> ourEncodedBytes(compressedLength + 16);
        size_t bytesWritten;
        error = JpegLsEncode(ourEncodedBytes.data(), ourEncodedBytes.size(), &bytesWritten, uncompressedData, uncompressedLength, &info, nullptr);
        if (error)
            return false;

        for (size_t i = 0; i < compressedLength; ++i)
        {
            if (static_cast<const uint8_t*>(compressedData)[i] != ourEncodedBytes[i])
            {
                return false;
            }
        }

        return true;
    }

    static bool verify_encoded_bytes(const vector<uint8_t>& uncompressed_source, const vector<uint8_t>& encoded_source)
    {
        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        jpegls_encoder encoder;
        encoder.frame_info(decoder.frame_info())
               .interleave_mode(decoder.interleave_mode())
               .near_lossless(decoder.near_lossless())
               .preset_coding_parameters(decoder.preset_coding_parameters());

        vector<uint8_t> ourEncodedBytes(encoded_source.size() + 16);
        encoder.destination(ourEncodedBytes);

        const size_t bytes_written = encoder.encode(uncompressed_source);

        if (bytes_written != encoded_source.size())
            return false;

        for (size_t i = 0; i < encoded_source.size(); ++i)
        {
            if (encoded_source[i] != ourEncodedBytes[i])
            {
                return false;
            }
        }

        return true;
    }
};

} // namespace CharLSUnitTest
