// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

// ReSharper disable CppClangTidyClangDiagnosticNonnull
#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include <array>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;
using std::vector;

MSVC_WARNING_SUPPRESS(6387) // '_Param_(x)' could be '0':  this does not adhere to the specification for the function.

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
#endif

namespace charls { namespace test {

TEST_CLASS(charls_jpegls_decoder_test)
{
public:
    TEST_METHOD(destroy_nullptr) // NOLINT
    {
        charls_jpegls_decoder_destroy(nullptr);

        // No explicit test possible, code should remain stable.
        Assert::IsTrue(true);
    }

    TEST_METHOD(set_source_buffer_nullptr) // NOLINT
    {
        const array<uint8_t, 10> buffer{};

        auto error = charls_jpegls_decoder_set_source_buffer(nullptr, buffer.data(), buffer.size());
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        auto* const decoder = charls_jpegls_decoder_create();
        error = charls_jpegls_decoder_set_source_buffer(decoder, nullptr, buffer.size());
        charls_jpegls_decoder_destroy(decoder);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(read_spiff_header_nullptr) // NOLINT
    {
        charls_spiff_header spiff_header{};
        int32_t header_found;
        auto error = charls_jpegls_decoder_read_spiff_header(nullptr, &spiff_header, &header_found);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};
        auto* decoder = charls_jpegls_decoder_create();
        error = charls_jpegls_decoder_set_source_buffer(decoder, source.data(), source.size());
        Assert::AreEqual(jpegls_errc::success, error);
        error = charls_jpegls_decoder_read_spiff_header(decoder, nullptr, &header_found);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
        charls_jpegls_decoder_destroy(decoder);

        decoder = charls_jpegls_decoder_create();
        error = charls_jpegls_decoder_set_source_buffer(decoder, source.data(), source.size());
        Assert::AreEqual(jpegls_errc::success, error);
        error = charls_jpegls_decoder_read_spiff_header(decoder, &spiff_header, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
        charls_jpegls_decoder_destroy(decoder);
    }

    TEST_METHOD(read_header_nullptr) // NOLINT
    {
        const auto error = charls_jpegls_decoder_read_header(nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);
    }

    TEST_METHOD(get_frame_info_nullptr) // NOLINT
    {
        frame_info frame_info;
        auto error = charls_jpegls_decoder_get_frame_info(nullptr, &frame_info);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto* decoder = get_initialized_decoder();
        error = charls_jpegls_decoder_get_frame_info(decoder, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }

    TEST_METHOD(get_near_lossless_nullptr) // NOLINT
    {
        int32_t near_lossless;
        auto error = charls_jpegls_decoder_get_near_lossless(nullptr, 0, &near_lossless);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto* decoder = get_initialized_decoder();
        error = charls_jpegls_decoder_get_near_lossless(decoder, 0, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }

    TEST_METHOD(get_interleave_mode_nullptr) // NOLINT
    {
        interleave_mode interleave_mode;
        auto error = charls_jpegls_decoder_get_interleave_mode(nullptr, &interleave_mode);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto* decoder = get_initialized_decoder();
        error = charls_jpegls_decoder_get_interleave_mode(decoder, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }

    TEST_METHOD(get_preset_coding_parameters_nullptr) // NOLINT
    {
        jpegls_pc_parameters preset_coding_parameters;
        auto error = charls_jpegls_decoder_get_preset_coding_parameters(nullptr, 0, &preset_coding_parameters);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto* decoder = get_initialized_decoder();
        error = charls_jpegls_decoder_get_preset_coding_parameters(decoder, 0, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }

    TEST_METHOD(get_destination_size_nullptr) // NOLINT
    {
        size_t destination_size_bytes;
        auto error = charls_jpegls_decoder_get_destination_size(nullptr, 0, &destination_size_bytes);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto* decoder = get_initialized_decoder();
        error = charls_jpegls_decoder_get_destination_size(decoder, 0, nullptr);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }

    TEST_METHOD(decode_to_buffer_nullptr) // NOLINT
    {
        array<uint8_t, 5> buffer{};
        auto error = charls_jpegls_decoder_decode_to_buffer(nullptr, buffer.data(), buffer.size(), 0);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        const auto* decoder = charls_jpegls_decoder_create();
        error = charls_jpegls_decoder_decode_to_buffer(decoder, nullptr, buffer.size(), 0);
        Assert::AreEqual(jpegls_errc::invalid_argument, error);

        charls_jpegls_decoder_destroy(decoder);
    }

private:
    static charls_jpegls_decoder* get_initialized_decoder()
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};
        auto* const decoder = charls_jpegls_decoder_create();
        auto error = charls_jpegls_decoder_set_source_buffer(decoder, source.data(), source.size());
        Assert::AreEqual(jpegls_errc::success, error);
        error = charls_jpegls_decoder_read_header(decoder);
        Assert::AreEqual(jpegls_errc::success, error);

        return decoder;
    }
};

}} // namespace charls::test

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

MSVC_WARNING_UNSUPPRESS()
