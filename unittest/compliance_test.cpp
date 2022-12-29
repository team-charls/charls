// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include "../test/portable_anymap_file.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using namespace charls_test;
using std::error_code;
using std::vector;

namespace charls { namespace test {

TEST_CLASS(compliance_test)
{
public:
    TEST_METHOD(decompress_color_8_bit_interleave_none_lossless) // NOLINT
    {
        // ISO 14495-1: official test image 1 (T87_test-1-2-3-4-5-6.zip)
        decompress_file("DataFiles/t8c0e0.jls", "DataFiles/test8.ppm");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_line_lossless) // NOLINT
    {
        // ISO 14495-1: official test image 2 (T87_test-1-2-3-4-5-6.zip)
        decompress_file("DataFiles/t8c1e0.jls", "DataFiles/test8.ppm");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_sample_lossless) // NOLINT
    {
        // ISO 14495-1: official test image 3 (T87_test-1-2-3-4-5-6.zip)
        decompress_file("DataFiles/t8c2e0.jls", "DataFiles/test8.ppm");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_none_near_lossless_3) // NOLINT
    {
        // ISO 14495-1: official test image 4 (T87_test-1-2-3-4-5-6.zip)
        decompress_file("DataFiles/t8c2e3.jls", "DataFiles/test8.ppm");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_line_near_lossless_3) // NOLINT
    {
        // ISO 14495-1: official test image 5 (T87_test-1-2-3-4-5-6.zip)
        decompress_file("DataFiles/t8c1e3.jls", "DataFiles/test8.ppm");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_sample_near_lossless_3) // NOLINT
    {
        // ISO 14495-1: official test image 6 (T87_test-1-2-3-4-5-6.zip)
        decompress_file("DataFiles/t8c2e3.jls", "DataFiles/test8.ppm");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_line_lossless_non_default) // NOLINT
    {
        // ISO 14495-1: official test image 9 (T87_test-1-2-3-4-5-6.zip)
        // NON-DEFAULT parameters T1=T2=T3=9,RESET=31.
        decompress_file("DataFiles/t8nde0.jls", "DataFiles/test8bs2.pgm");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_line_near_lossless_3_non_default) // NOLINT
    {
        // ISO 14495-1: official test image 10 (T87_test-1-2-3-4-5-6.zip)
        // NON-DEFAULT parameters T1=T2=T3=9,RESET=31.
        decompress_file("DataFiles/t8nde3.jls", "DataFiles/test8bs2.pgm");
    }

    TEST_METHOD(decompress_monochrome_16_bit_lossless) // NOLINT
    {
        // ISO 14495-1: official test image 11 (T87_test-11-12.zip)
        // Note: test image is actually 12 bit.
        decompress_file("DataFiles/t16e0.jls", "DataFiles/test16.pgm");
    }

    TEST_METHOD(decompress_monochrome_16_bit_near_lossless_3) // NOLINT
    {
        // ISO 14495-1: official test image 12 (T87_test-11-12.zip)
        // Note: test image is actually 12 bit.
        decompress_file("DataFiles/t16e3.jls", "DataFiles/TEST16.pgm", false);
    }

    TEST_METHOD(tulips_monochrome_8_bit_lossless_hp) // NOLINT
    {
        // Additional, Tulips encoded with HP 1.0BETA encoder.
        decompress_file("DataFiles/tulips-gray-8bit-512-512-hp-encoder.jls", "DataFiles/tulips-gray-8bit-512-512.pgm");
    }

    TEST_METHOD(decompress_color_8_bit_interleave_none_lossless_restart_7) // NOLINT
    {
        // ISO 14495-1: official test image 1 but with restart markers.
        decompress_file("DataFiles/test8_ilv_none_rm_7.jls", "DataFiles/test8.ppm", false);
    }

    TEST_METHOD(decompress_color_8_bit_interleave_line_lossless_restart_7) // NOLINT
    {
        // ISO 14495-1: official test image 2 but with restart markers.
        decompress_file("DataFiles/test8_ilv_line_rm_7.jls", "DataFiles/test8.ppm", false);
    }

    TEST_METHOD(decompress_color_8_bit_interleave_sample_lossless_restart_7) // NOLINT
    {
        // ISO 14495-1: official test image 3 but with restart markers.
        decompress_file("DataFiles/test8_ilv_sample_rm_7.jls", "DataFiles/test8.ppm", false);
    }

    TEST_METHOD(decompress_color_8_bit_interleave_sample_lossless_restart_300) // NOLINT
    {
        // ISO 14495-1: official test image 3 but with restart markers and restart interval 300
        decompress_file("DataFiles/test8_ilv_sample_rm_300.jls", "DataFiles/test8.ppm", false);
    }

    TEST_METHOD(decompress_monochrome_16_bit_restart_5) // NOLINT
    {
        // ISO 14495-1: official test image 12 but with restart markers and restart interval 5
        decompress_file("DataFiles/test16_rm_5.jls", "DataFiles/test16.pgm", false);
    }

private:
    static void decompress_file(const char* encoded_filename, const char* raw_filename, const bool check_encode = true)
    {
        const vector<uint8_t> encoded_source{read_file(encoded_filename)};
        const jpegls_decoder decoder{encoded_source, true};

        portable_anymap_file reference_file{
            read_anymap_reference_file(raw_filename, decoder.interleave_mode(), decoder.frame_info())};

        test_compliance(encoded_source, reference_file.image_data(), check_encode);
        test_compliance_legacy_api(encoded_source.data(), encoded_source.size(), reference_file.image_data().data(),
                                   reference_file.image_data().size(), check_encode);
    }

    static void test_compliance_legacy_api(const uint8_t* compressed_bytes, const size_t compressed_length,
                                           const uint8_t* uncompressed_data, const size_t uncompressed_length,
                                           const bool check_encode)
    {
        // ReSharper disable CppDeprecatedEntity
        DISABLE_DEPRECATED_WARNING

        JlsParameters info{};
        error_code error{JpegLsReadHeader(compressed_bytes, compressed_length, &info, nullptr)};
        Assert::IsFalse(static_cast<bool>(error));

        if (check_encode)
        {
            Assert::IsTrue(verify_encoded_bytes_legacy_api(uncompressed_data, uncompressed_length, compressed_bytes,
                                                           compressed_length));
        }

        vector<uint8_t> destination(static_cast<size_t>(info.height) * info.width * bit_to_byte_count(info.bitsPerSample) *
                                    info.components);

        error = JpegLsDecode(destination.data(), destination.size(), compressed_bytes, compressed_length, nullptr, nullptr);
        Assert::IsTrue(!error);

        if (info.allowedLossyError == 0)
        {
            for (size_t i{}; i != uncompressed_length; ++i)
            {
                if (uncompressed_data[i] != destination[i]) // AreEqual is very slow, pre-test to speed up 50X
                {
                    Assert::AreEqual(uncompressed_data[i], destination[i]);
                }
            }
        }

        // ReSharper restore CppDeprecatedEntity
        RESTORE_DEPRECATED_WARNING
    }

    static bool verify_encoded_bytes_legacy_api(const void* uncompressed_data, const size_t uncompressed_length,
                                                const void* compressed_data, const size_t compressed_length)
    {
        // ReSharper disable CppDeprecatedEntity
        DISABLE_DEPRECATED_WARNING

        JlsParameters info{};
        error_code error{JpegLsReadHeader(compressed_data, compressed_length, &info, nullptr)};
        if (error)
            return false;

        vector<uint8_t> our_encoded_bytes(compressed_length + 16);
        size_t bytes_written;
        error = JpegLsEncode(our_encoded_bytes.data(), our_encoded_bytes.size(), &bytes_written, uncompressed_data,
                             uncompressed_length, &info, nullptr);
        if (error)
            return false;

        for (size_t i{}; i != compressed_length; ++i)
        {
            if (static_cast<const uint8_t*>(compressed_data)[i] != our_encoded_bytes[i])
            {
                return false;
            }
        }

        return true;

        // ReSharper restore CppDeprecatedEntity
        RESTORE_DEPRECATED_WARNING
    }
};

}} // namespace charls::test
