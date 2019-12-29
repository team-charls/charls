// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using namespace charls;


namespace CharLSUnitTest {

// clang-format off

TEST_CLASS(charls_jpegls_decoder_test)
{
public:
    TEST_METHOD(destroy_nullptr)
    {
        charls_jpegls_decoder_destroy(nullptr);

        // No explicit test possible, code should remain stable.
        Assert::IsTrue(true);
    }

    TEST_METHOD(set_source_buffer_nullptr)
    {
        const uint8_t buffer[10]{};

        auto error = charls_jpegls_decoder_set_source_buffer(nullptr, buffer, sizeof buffer);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto decoder = charls_jpegls_decoder_create();
        error = charls_jpegls_decoder_set_source_buffer(decoder, nullptr, sizeof buffer);
        charls_jpegls_decoder_destroy(decoder);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(read_spiff_header_nullptr)
    {
        charls_spiff_header spiff_header;
        int32_t header_found;
        auto error = charls_jpegls_decoder_read_spiff_header(nullptr, &spiff_header, &header_found);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto decoder = charls_jpegls_decoder_create();
        error = charls_jpegls_decoder_read_spiff_header(decoder, nullptr, &header_found);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        error = charls_jpegls_decoder_read_spiff_header(decoder, &spiff_header, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }

    TEST_METHOD(read_header_nullptr)
    {
        const auto error = charls_jpegls_decoder_read_header(nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(get_frame_info_nullptr)
    {
        frame_info frame_info;
        auto error = charls_jpegls_decoder_get_frame_info(nullptr, &frame_info);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto* decoder = charls_jpegls_decoder_create();
        error = charls_jpegls_decoder_get_frame_info(decoder, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }

    TEST_METHOD(get_near_lossless_nullptr)
    {
        int32_t near_lossless;
        auto error = charls_jpegls_decoder_get_near_lossless(nullptr, 0, &near_lossless);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto* decoder = charls_jpegls_decoder_create();
        error = charls_jpegls_decoder_get_near_lossless(decoder, 0, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }

    TEST_METHOD(get_interleave_mode_nullptr)
    {
        interleave_mode interleave_mode;
        auto error = charls_jpegls_decoder_get_interleave_mode(nullptr, &interleave_mode);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto* decoder = charls_jpegls_decoder_create();
        error = charls_jpegls_decoder_get_interleave_mode(decoder, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }

    TEST_METHOD(get_preset_coding_parameters_nullptr)
    {
        jpegls_pc_parameters preset_coding_parameters;
        auto error = charls_jpegls_decoder_get_preset_coding_parameters(nullptr, 0, &preset_coding_parameters);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto* decoder = charls_jpegls_decoder_create();
        error = charls_jpegls_decoder_get_preset_coding_parameters(decoder, 0, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }

    TEST_METHOD(get_destination_size_nullptr)
    {
        size_t destination_size_bytes;
        auto error = charls_jpegls_decoder_get_destination_size(nullptr, 0, &destination_size_bytes);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto* decoder = charls_jpegls_decoder_create();
        error = charls_jpegls_decoder_get_destination_size(decoder, 0, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }

    TEST_METHOD(decode_to_buffer_nullptr)
    {
        uint8_t buffer[5];
        auto error = charls_jpegls_decoder_decode_to_buffer(nullptr, buffer, 5, 0);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto* decoder = charls_jpegls_decoder_create();
        error = charls_jpegls_decoder_decode_to_buffer(decoder, nullptr, 5, 0);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }
};

}
