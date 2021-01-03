// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include <array>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;

MSVC_WARNING_SUPPRESS(6387) // '_Param_(x)' could be '0': this does not adhere to the specification for the function.

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
#endif

namespace charls { namespace test {

TEST_CLASS(charls_jpegls_encoder_test)
{
public:
    TEST_METHOD(destroy_nullptr) // NOLINT
    {
        charls_jpegls_encoder_destroy(nullptr);

        // No explicit test possible, code should remain stable.
        Assert::IsTrue(true);
    }

    TEST_METHOD(set_destination_buffer_nullptr) // NOLINT
    {
        array<uint8_t, 10> buffer{};
        auto error = charls_jpegls_encoder_set_destination_buffer(nullptr, buffer.data(), buffer.size());
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        auto* const encoder = charls_jpegls_encoder_create();
        error = charls_jpegls_encoder_set_destination_buffer(encoder, nullptr, buffer.size());
        charls_jpegls_encoder_destroy(encoder);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(set_frame_info_buffer_nullptr) // NOLINT
    {
        charls_frame_info frame_info{};
        auto error = charls_jpegls_encoder_set_frame_info(nullptr, &frame_info);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        auto* const encoder = charls_jpegls_encoder_create();
        error = charls_jpegls_encoder_set_frame_info(encoder, nullptr);
        charls_jpegls_encoder_destroy(encoder);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(set_near_lossless_nullptr) // NOLINT
    {
        const auto error = charls_jpegls_encoder_set_near_lossless(nullptr, 1);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(set_interleave_mode_nullptr) // NOLINT
    {
        const auto error = charls_jpegls_encoder_set_interleave_mode(nullptr, charls_interleave_mode::line);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(set_preset_coding_parameters_nullptr) // NOLINT
    {
        charls_jpegls_pc_parameters parameters{};
        auto error = charls_jpegls_encoder_set_preset_coding_parameters(nullptr, &parameters);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        auto* const encoder = charls_jpegls_encoder_create();
        error = charls_jpegls_encoder_set_preset_coding_parameters(encoder, nullptr);
        charls_jpegls_encoder_destroy(encoder);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(set_color_transformation_nullptr) // NOLINT
    {
        const auto error = charls_jpegls_encoder_set_color_transformation(nullptr, charls_color_transformation::hp1);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(get_estimated_destination_size_nullptr) // NOLINT
    {
        size_t size_in_bytes{};
        auto error = charls_jpegls_encoder_get_estimated_destination_size(nullptr, &size_in_bytes);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_encoder* const encoder = charls_jpegls_encoder_create();

        charls_frame_info frame_info{1, 1, 2, 1};
        error = charls_jpegls_encoder_set_frame_info(encoder, &frame_info);
        Assert::AreEqual(jpegls_errc::success, error);

        error = charls_jpegls_encoder_get_estimated_destination_size(encoder, nullptr);
        charls_jpegls_encoder_destroy(encoder);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(get_bytes_written_nullptr) // NOLINT
    {
        size_t bytes_written{};
        auto error = charls_jpegls_encoder_get_bytes_written(nullptr, &bytes_written);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_encoder const* const encoder = charls_jpegls_encoder_create();
        error = charls_jpegls_encoder_get_bytes_written(encoder, nullptr);
        charls_jpegls_encoder_destroy(encoder);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(encode_from_buffer_nullptr) // NOLINT
    {
        const array<uint8_t, 10> source_buffer{};
        auto error = charls_jpegls_encoder_encode_from_buffer(nullptr, source_buffer.data(), source_buffer.size(), 0);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        auto* const encoder = charls_jpegls_encoder_create();
        error = charls_jpegls_encoder_encode_from_buffer(encoder, nullptr, source_buffer.size(), 0);
        charls_jpegls_encoder_destroy(encoder);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(write_spiff_header_nullptr) // NOLINT
    {
        charls_spiff_header spiff_header{};
        auto error = charls_jpegls_encoder_write_spiff_header(nullptr, &spiff_header);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        auto* const encoder = charls_jpegls_encoder_create();
        error = charls_jpegls_encoder_write_spiff_header(encoder, nullptr);
        charls_jpegls_encoder_destroy(encoder);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(write_standard_spiff_header_nullptr) // NOLINT
    {
        const auto error = charls_jpegls_encoder_write_standard_spiff_header(
            nullptr, charls_spiff_color_space::cie_lab, charls_spiff_resolution_units::dots_per_centimeter, 1, 1);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(write_spiff_entry_nullptr) // NOLINT
    {
        const array<uint8_t, 10> entry_data{};
        auto error = charls_jpegls_encoder_write_spiff_entry(nullptr, 5, entry_data.data(), entry_data.size());
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        auto* const encoder = charls_jpegls_encoder_create();
        error = charls_jpegls_encoder_write_spiff_entry(encoder, 5, nullptr, sizeof(entry_data));
        charls_jpegls_encoder_destroy(encoder);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }
};

}} // namespace charls::test

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

MSVC_WARNING_UNSUPPRESS()
