// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>
#include <charls/charls_legacy.h>

#include <array>
#include <vector>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using namespace charls;
using std::array;
using std::vector;

// clang-format off

namespace CharLSUnitTest {

TEST_CLASS(interface_test)
{
public:
    TEST_METHOD(GetMetadataInfoFromNearLosslessEncodedColorImage)
    {
        vector<uint8_t> encoded_source{read_file("DataFiles/T8C0E3.JLS")};

        JlsParameters params{};
        const jpegls_errc result = JpegLsReadHeader(encoded_source.data(), encoded_source.size(), &params, nullptr);

        Assert::AreEqual(jpegls_errc::success, result);
        Assert::AreEqual(params.height, 256);
        Assert::AreEqual(params.width, 256);
        Assert::AreEqual(params.bitsPerSample, 8);
        Assert::AreEqual(params.components, 3);
        Assert::AreEqual(params.allowedLossyError, 3);
    }

    TEST_METHOD(JpegLsReadHeader_nullptr)
    {
        JlsParameters params{};
        vector<uint8_t> buffer(100);
        auto error = JpegLsReadHeader(nullptr, buffer.size(), &params, nullptr);
        Assert::AreEqual(charls::jpegls_errc::invalid_argument, error);

        error = JpegLsReadHeader(buffer.data(), buffer.size(), nullptr, nullptr);
        Assert::AreEqual(charls::jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(JpegLsReadHeader_empty_source)
    {
        array<char, ErrorMessageSize> error_message;
        JlsParameters params{};
        array<uint8_t, 1> source;
        const auto error = JpegLsReadHeader(source.data(), 0, &params, error_message.data());
        Assert::AreEqual(charls::jpegls_errc::source_buffer_too_small, error);
        Assert::IsTrue(strlen(error_message.data()) > 0);
    }

    TEST_METHOD(JpegLsReadHeader_custom_preset_parameters)
    {
        // NON-DEFAULT parameters T1=T2=T3=9,RESET=31.
        vector<uint8_t> encoded_source{read_file("DataFiles/T8NDE0.JLS")};

        JlsParameters params{};
        const jpegls_errc result = JpegLsReadHeader(encoded_source.data(), encoded_source.size(), &params, nullptr);

        Assert::AreEqual(jpegls_errc::success, result);
        Assert::AreEqual(255, params.custom.MaximumSampleValue);
        Assert::AreEqual(9, params.custom.Threshold1);
        Assert::AreEqual(9, params.custom.Threshold2);
        Assert::AreEqual(9, params.custom.Threshold3);
        Assert::AreEqual(31, params.custom.ResetValue);
    }

    TEST_METHOD(JpegLsEncode_nullptr)
    {
        JlsParameters params{};
        size_t bytesWritten{};
        vector<uint8_t> buffer(100);
        auto error = JpegLsEncode(nullptr, 100, &bytesWritten, buffer.data(), buffer.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        error = JpegLsEncode(buffer.data(), 100, nullptr, buffer.data(), buffer.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        error = JpegLsEncode(buffer.data(), buffer.size(),  &bytesWritten, nullptr, buffer.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        error = JpegLsEncode(buffer.data(), buffer.size(),  &bytesWritten, buffer.data(), buffer.size(), nullptr, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(JpegLsEncode_empty_destination)
    {
        array<char, ErrorMessageSize> error_message;

        JlsParameters params{};
        params.bitsPerSample = 8;
        params.height = 10;
        params.width = 10;
        params.components = 1;

        size_t bytesWritten{};
        array<uint8_t, 1> destination;
        vector<uint8_t> source(100);
        const auto error = JpegLsEncode(destination.data(), 0, &bytesWritten, source.data(), source.size(), &params, error_message.data());
        Assert::AreEqual(charls::jpegls_errc::destination_buffer_too_small, error);
        Assert::IsTrue(strlen(error_message.data()) > 0);
    }

    TEST_METHOD(JpegLsDecode_nullptr)
    {
        JlsParameters params{};
        vector<uint8_t> buffer(100);
        auto error = JpegLsDecode(nullptr, 100, buffer.data(), buffer.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        error = JpegLsDecode(buffer.data(), 100, nullptr, buffer.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(JpegLsDecode_empty_source)
    {
        array<char, ErrorMessageSize> error_message;
        JlsParameters params{};
        params.bitsPerSample = 8;
        params.height = 10;
        params.width = 10;
        params.components = 1;

        array<uint8_t, 1> source;
        vector<uint8_t> destination(100);
        const auto error = JpegLsDecode(destination.data(), destination.size(), source.data(), 0, &params, error_message.data());
        Assert::AreEqual(jpegls_errc::source_buffer_too_small, error);
        Assert::IsTrue(strlen(error_message.data()) > 0);
    }

    TEST_METHOD(JpegLsDecodeRect_lena)
    {
        JlsParameters params{};
        vector<uint8_t> encoded_source = read_file("DataFiles/lena8b.jls");
        auto error = JpegLsReadHeader(encoded_source.data(), encoded_source.size(), &params, nullptr);
        Assert::AreEqual(jpegls_errc::success, error);

        vector<uint8_t> decoded_destination(static_cast<size_t>(params.width) * params.height*params.components);

        error = JpegLsDecode(decoded_destination.data(), decoded_destination.size(),
            encoded_source.data(), encoded_source.size(), &params, nullptr);
        Assert::IsFalse(static_cast<bool>(error));

        const JlsRect rect = { 128, 128, 256, 1 };
        vector<uint8_t> decoded_rect(static_cast<size_t>(rect.Width) * rect.Height);
        decoded_rect.push_back(0x1f);
        error = JpegLsDecodeRect(decoded_rect.data(), decoded_rect.size(),
            encoded_source.data(), encoded_source.size(), rect, &params, nullptr);
        Assert::IsFalse(static_cast<bool>(error));

        Assert::IsTrue(memcmp(&decoded_destination[rect.X + static_cast<size_t>(rect.Y) * 512], decoded_rect.data(), static_cast<size_t>(rect.Width) * rect.Height) == 0);
        Assert::IsTrue(decoded_rect[static_cast<size_t>(rect.Width) * rect.Height] == 0x1f);
    }

    TEST_METHOD(JpegLsDecodeRect_nullptr)
    {
        JlsParameters params{};
        const JlsRect roi{};
        vector<uint8_t> buffer(100);
        auto error = JpegLsDecodeRect(nullptr, 100, buffer.data(), buffer.size(), roi, &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        error = JpegLsDecodeRect(buffer.data(), 100, nullptr, buffer.size(), roi, &params, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(JpegLsDecodeRect_empty_source)
    {
        array<char, ErrorMessageSize> error_message;
        JlsParameters params{};
        params.bitsPerSample = 8;
        params.height = 10;
        params.width = 10;
        params.components = 1;

        const JlsRect roi{};
        array<uint8_t, 1> source;
        vector<uint8_t> destination(100);

        const auto error = JpegLsDecodeRect(destination.data(), destination.size(), source.data(), 0, roi, &params, error_message.data());
        Assert::AreEqual(jpegls_errc::source_buffer_too_small, error);
        Assert::IsTrue(strlen(error_message.data()) > 0);
    }

    TEST_METHOD(noise_image_with_custom_reset)
    {
        JlsParameters params{};
        params.components = 1;
        params.bitsPerSample = 16;
        params.height = 512;
        params.width = 512;
        params.custom.MaximumSampleValue = (1 << params.bitsPerSample) - 1;
        params.custom.ResetValue = 63;

        const vector<uint8_t> noise_image = create_noise_image_16bit(static_cast<size_t>(params.height) * params.width, params.bitsPerSample, 21344);

        test_round_trip_legacy(noise_image, params);
    }

    TEST_METHOD(JpegLsReadHeaderStream_valid_input)
    {
        vector<uint8_t> source{read_file("DataFiles/T8C0E3.JLS")};

        JlsParameters params{};
        const auto source_info = FromByteArrayConst(source.data(), source.size());
        const jpegls_errc error = JpegLsReadHeaderStream(source_info, &params);

        Assert::AreEqual(jpegls_errc::success, error);
        Assert::AreEqual(params.height, 256);
        Assert::AreEqual(params.width, 256);
        Assert::AreEqual(params.bitsPerSample, 8);
        Assert::AreEqual(params.components, 3);
        Assert::AreEqual(params.allowedLossyError, 3);
    }

    TEST_METHOD(JpegLsDecodeStream_valid_input)
    {
        vector<uint8_t> source{read_file("DataFiles/T8C0E3.JLS")};

        JlsParameters params{};
        const auto source_info = FromByteArrayConst(source.data(), source.size());
        jpegls_errc error = JpegLsReadHeaderStream(source_info, &params);
        Assert::AreEqual(jpegls_errc::success, error);

        const int bytesPerSample = params.bitsPerSample > 8 ? 2 : 1;
        vector<uint8_t> destination(static_cast<size_t>(params.width) * params.height * bytesPerSample * params.components);

        const auto destination_info = FromByteArray(destination.data(), destination.size());
        error = JpegLsDecodeStream(destination_info, source_info, nullptr);
        Assert::AreEqual(jpegls_errc::success, error);
    }
};

} // namespace CharLSUnitTest
