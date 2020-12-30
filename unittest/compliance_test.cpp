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

    TEST_METHOD(lena_monochrome_8_bit_lossless_ubc) // NOLINT
    {
        // Additional, Lena compressed with other codec (UBC?)
        decompress_file("DataFiles/lena8b.jls", "DataFiles/lena8b.pgm");
    }

private:
    static void decompress_file(const char* encoded_filename, const char* raw_filename, const bool check_encode = true)
    {
        vector<uint8_t> encoded_source = read_file(encoded_filename);

        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        portable_anymap_file reference_file =
            read_anymap_reference_file(raw_filename, decoder.interleave_mode(), decoder.frame_info());

        test_compliance(encoded_source, reference_file.image_data(), check_encode);
        test_compliance_legacy_api(encoded_source.data(), encoded_source.size(), reference_file.image_data().data(),
                                   reference_file.image_data().size(), check_encode);
    }

    static void test_compliance(const vector<uint8_t>& encoded_source, const vector<uint8_t>& uncompressed_source,
                                const bool check_encode)
    {
        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        if (check_encode)
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
                    if (abs(uncompressed_source[i] - destination[i]) >
                        near_lossless) // AreEqual is very slow, pre-test to speed up 50X
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
                    if (abs(source16[i] - destination16[i]) >
                        near_lossless) // AreEqual is very slow, pre-test to speed up 50X
                    {
                        Assert::AreEqual(static_cast<int>(source16[i]), static_cast<int>(destination16[i]));
                    }
                }
            }
        }
    }

    static void test_compliance_legacy_api(const uint8_t* compressed_bytes, const size_t compressed_length,
                                           const uint8_t* uncompressed_data, const size_t uncompressed_length,
                                           const bool check_encode)
    {
        // ReSharper disable CppDeprecatedEntity
        DISABLE_DEPRECATED_WARNING

        JlsParameters info{};
        error_code error = JpegLsReadHeader(compressed_bytes, compressed_length, &info, nullptr);
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
            for (size_t i = 0; i < uncompressed_length; ++i)
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
        error_code error = JpegLsReadHeader(compressed_data, compressed_length, &info, nullptr);
        if (error)
            return false;

        vector<uint8_t> our_encoded_bytes(compressed_length + 16);
        size_t bytes_written;
        error = JpegLsEncode(our_encoded_bytes.data(), our_encoded_bytes.size(), &bytes_written, uncompressed_data,
                             uncompressed_length, &info, nullptr);
        if (error)
            return false;

        for (size_t i = 0; i < compressed_length; ++i)
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

        vector<uint8_t> our_encoded_bytes(encoded_source.size() + 16);
        encoder.destination(our_encoded_bytes);

        const size_t bytes_written = encoder.encode(uncompressed_source);

        if (bytes_written != encoded_source.size())
            return false;

        for (size_t i = 0; i < encoded_source.size(); ++i)
        {
            if (encoded_source[i] != our_encoded_bytes[i])
            {
                return false;
            }
        }

        return true;
    }
};

}} // namespace charls::test
