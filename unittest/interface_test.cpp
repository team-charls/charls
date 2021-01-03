// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include <array>
#include <vector>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;
using std::vector;

MSVC_WARNING_SUPPRESS(6387) // '_Param_(x)' could be '0': this does not adhere to the specification for the function.

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
#endif

// ReSharper disable CppDeprecatedEntity
DISABLE_DEPRECATED_WARNING

namespace charls { namespace test {

TEST_CLASS(interface_test)
{
public:
    TEST_METHOD(get_metadata_info_from_near_lossless_encoded_color_image) // NOLINT
    {
        vector<uint8_t> encoded_source{read_file("DataFiles/t8c0e3.jls")};

        JlsParameters params{};
        const jpegls_errc result = JpegLsReadHeader(encoded_source.data(), encoded_source.size(), &params, nullptr);

        Assert::AreEqual(jpegls_errc::success, result);
        Assert::AreEqual(params.height, 256);
        Assert::AreEqual(params.width, 256);
        Assert::AreEqual(params.bitsPerSample, 8);
        Assert::AreEqual(params.components, 3);
        Assert::AreEqual(params.allowedLossyError, 3);
    }

    TEST_METHOD(JpegLsReadHeader_nullptr) // NOLINT
    {
        JlsParameters params{};
        vector<uint8_t> encoded_source{read_file("DataFiles/t8c0e3.jls")};
        auto error = JpegLsReadHeader(nullptr, encoded_source.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        error = JpegLsReadHeader(encoded_source.data(), encoded_source.size(), nullptr, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(JpegLsReadHeader_empty_source) // NOLINT
    {
        array<char, ErrorMessageSize> error_message{};
        JlsParameters params{};
        array<uint8_t, 1> source{};
        const auto error = JpegLsReadHeader(source.data(), 0, &params, error_message.data());
        Assert::AreEqual(jpegls_errc::source_buffer_too_small, error);
        Assert::IsTrue(strlen(error_message.data()) > 0);
    }

    TEST_METHOD(JpegLsReadHeader_custom_preset_parameters) // NOLINT
    {
        // NON-DEFAULT parameters T1=T2=T3=9,RESET=31.
        vector<uint8_t> encoded_source{read_file("DataFiles/t8nde0.jls")};

        JlsParameters params{};
        const jpegls_errc result = JpegLsReadHeader(encoded_source.data(), encoded_source.size(), &params, nullptr);

        Assert::AreEqual(jpegls_errc::success, result);
        Assert::AreEqual(255, params.custom.MaximumSampleValue);
        Assert::AreEqual(9, params.custom.Threshold1);
        Assert::AreEqual(9, params.custom.Threshold2);
        Assert::AreEqual(9, params.custom.Threshold3);
        Assert::AreEqual(31, params.custom.ResetValue);
    }

    TEST_METHOD(JpegLsEncode_nullptr) // NOLINT
    {
        JlsParameters params{};
        params.bitsPerSample = 8;
        params.height = 10;
        params.width = 10;
        params.components = 1;

        size_t bytes_written{};
        vector<uint8_t> buffer(10000);
        auto error = JpegLsEncode(nullptr, buffer.size(), &bytes_written, buffer.data(), buffer.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        error = JpegLsEncode(buffer.data(), buffer.size(), nullptr, buffer.data(), buffer.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        error = JpegLsEncode(buffer.data(), buffer.size(), &bytes_written, nullptr, buffer.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        error = JpegLsEncode(buffer.data(), buffer.size(), &bytes_written, buffer.data(), buffer.size(), nullptr, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(JpegLsEncode_empty_destination) // NOLINT
    {
        array<char, ErrorMessageSize> error_message{};

        JlsParameters params{};
        params.bitsPerSample = 8;
        params.height = 10;
        params.width = 10;
        params.components = 1;

        size_t bytes_written{};
        array<uint8_t, 1> destination{};
        vector<uint8_t> source(100);
        const auto error =
            JpegLsEncode(destination.data(), 0, &bytes_written, source.data(), source.size(), &params, error_message.data());
        Assert::AreEqual(jpegls_errc::destination_buffer_too_small, error);
        Assert::IsTrue(strlen(error_message.data()) > 0);
    }

    TEST_METHOD(JpegLsDecode_nullptr) // NOLINT
    {
        JlsParameters params{};
        vector<uint8_t> encoded_source = read_file("DataFiles/lena8b.jls");
        auto error = JpegLsDecode(nullptr, 100, encoded_source.data(), encoded_source.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        error = JpegLsDecode(encoded_source.data(), 100, nullptr, encoded_source.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(JpegLsDecode_empty_source) // NOLINT
    {
        array<char, ErrorMessageSize> error_message{};
        JlsParameters params{};
        params.bitsPerSample = 8;
        params.height = 10;
        params.width = 10;
        params.components = 1;

        array<uint8_t, 1> source{};
        vector<uint8_t> destination(100);
        const auto error =
            JpegLsDecode(destination.data(), destination.size(), source.data(), 0, &params, error_message.data());
        Assert::AreEqual(jpegls_errc::source_buffer_too_small, error);
        Assert::IsTrue(strlen(error_message.data()) > 0);
    }

    TEST_METHOD(JpegLsDecodeRect_lena) // NOLINT
    {
        JlsParameters params{};
        vector<uint8_t> encoded_source = read_file("DataFiles/lena8b.jls");
        auto error = JpegLsReadHeader(encoded_source.data(), encoded_source.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::success, error);

        vector<uint8_t> decoded_destination(static_cast<size_t>(params.width) * params.height * params.components);

        error = JpegLsDecode(decoded_destination.data(), decoded_destination.size(), encoded_source.data(),
                             encoded_source.size(), &params, nullptr);
        Assert::IsFalse(static_cast<bool>(error));

        const JlsRect rect = {128, 128, 256, 1};
        vector<uint8_t> decoded_rect(static_cast<size_t>(rect.Width) * rect.Height);
        decoded_rect.push_back(0x1f);
        error = JpegLsDecodeRect(decoded_rect.data(), decoded_rect.size(), encoded_source.data(), encoded_source.size(),
                                 rect, &params, nullptr);
        Assert::IsFalse(static_cast<bool>(error));

        Assert::IsTrue(memcmp(&decoded_destination[rect.X + static_cast<size_t>(rect.Y) * 512], decoded_rect.data(),
                              static_cast<size_t>(rect.Width) * rect.Height) == 0);
        Assert::IsTrue(decoded_rect[static_cast<size_t>(rect.Width) * rect.Height] == 0x1f);
    }

    TEST_METHOD(JpegLsDecodeRect_nullptr) // NOLINT
    {
        JlsParameters params{};
        const JlsRect roi{};
        vector<uint8_t> encoded_source = read_file("DataFiles/lena8b.jls");
        auto error = JpegLsDecodeRect(nullptr, 100, encoded_source.data(), encoded_source.size(), roi, &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        error = JpegLsDecodeRect(encoded_source.data(), 100, nullptr, encoded_source.size(), roi, &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(JpegLsDecodeRect_empty_source) // NOLINT
    {
        array<char, ErrorMessageSize> error_message{};
        JlsParameters params{};
        params.bitsPerSample = 8;
        params.height = 10;
        params.width = 10;
        params.components = 1;

        const JlsRect roi{};
        array<uint8_t, 1> source{};
        vector<uint8_t> destination(100);

        const auto error =
            JpegLsDecodeRect(destination.data(), destination.size(), source.data(), 0, roi, &params, error_message.data());
        Assert::AreEqual(jpegls_errc::source_buffer_too_small, error);
        Assert::IsTrue(strlen(error_message.data()) > 0);
    }

    TEST_METHOD(noise_image_with_custom_reset) // NOLINT
    {
        JlsParameters params{};
        params.components = 1;
        params.bitsPerSample = 16;
        params.height = 512;
        params.width = 512;
        params.custom.MaximumSampleValue = (1 << params.bitsPerSample) - 1;
        params.custom.ResetValue = 63;

        const vector<uint8_t> noise_image =
            create_noise_image_16_bit(static_cast<size_t>(params.height) * params.width, params.bitsPerSample, 21344);

        test_round_trip_legacy(noise_image, params);
    }
};

}} // namespace charls::test

// ReSharper restore CppDeprecatedEntity
RESTORE_DEPRECATED_WARNING

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

MSVC_WARNING_UNSUPPRESS()
