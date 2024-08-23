// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "util.hpp"

#include <charls/validate_spiff_header.h>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using Microsoft::VisualStudio::CppUnitTestFramework::TestClass;

MSVC_WARNING_SUPPRESS(6387) // '_Param_(x)' could be '0': this does not adhere to the specification for the function.

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
#endif

namespace charls::test {

namespace {

constexpr spiff_header create_valid_spiff_header()
{
    return {
        spiff_profile_id::none,
        3,
        200,
        100,
        spiff_color_space::rgb,
        8,
        spiff_compression_type::jpeg_ls,
        spiff_resolution_units::aspect_ratio,
        1,
        1,
    };
}

constexpr frame_info create_valid_frame_info()
{
    return {100, 200, 8, 3};
}

} // namespace

TEST_CLASS(charls_validate_spiff_header_test)
{
public:
    TEST_METHOD(valid) // NOLINT
    {
        spiff_header spiff_header{create_valid_spiff_header()};
        constexpr frame_info frame_info{create_valid_frame_info()};

        auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        Assert::AreEqual(jpegls_errc::success, result);

        spiff_header.color_space = spiff_color_space::none;
        result = charls_validate_spiff_header(&spiff_header, &frame_info);
        Assert::AreEqual(jpegls_errc::success, result);
    }

    TEST_METHOD(invalid_compression_type) // NOLINT
    {
        spiff_header spiff_header{create_valid_spiff_header()};
        constexpr frame_info frame_info{create_valid_frame_info()};
        spiff_header.compression_type = spiff_compression_type::uncompressed;

        const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        Assert::AreEqual(jpegls_errc::invalid_spiff_header, result);
    }

    TEST_METHOD(invalid_profile_id) // NOLINT
    {
        spiff_header spiff_header{};
        constexpr frame_info frame_info{};
        spiff_header.compression_type = spiff_compression_type::jpeg_ls;
        spiff_header.profile_id = spiff_profile_id::continuous_tone_base;

        const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        Assert::AreEqual(jpegls_errc::invalid_spiff_header, result);
    }

    TEST_METHOD(invalid_component_count) // NOLINT
    {
        spiff_header spiff_header{create_valid_spiff_header()};
        constexpr frame_info frame_info{create_valid_frame_info()};
        spiff_header.component_count = 7;

        const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        Assert::AreEqual(jpegls_errc::invalid_spiff_header, result);
    }

    TEST_METHOD(all_jpegls_bits_per_sample_are_valid) // NOLINT
    {
        spiff_header spiff_header{create_valid_spiff_header()};
        frame_info frame_info{create_valid_frame_info()};

        for (int bits_per_sample{2}; bits_per_sample <= 16; ++bits_per_sample)
        {
            spiff_header.bits_per_sample = bits_per_sample;
            frame_info.bits_per_sample = bits_per_sample;

            const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
            Assert::AreEqual(jpegls_errc::success, result);
        }
    }

    TEST_METHOD(invalid_bits_per_sample) // NOLINT
    {
        spiff_header spiff_header{create_valid_spiff_header()};
        constexpr frame_info frame_info{create_valid_frame_info()};
        spiff_header.bits_per_sample = 12;

        const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        Assert::AreEqual(jpegls_errc::invalid_spiff_header, result);
    }

    TEST_METHOD(invalid_height) // NOLINT
    {
        spiff_header spiff_header{create_valid_spiff_header()};
        constexpr frame_info frame_info{create_valid_frame_info()};
        spiff_header.height = 333;

        const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        Assert::AreEqual(jpegls_errc::invalid_spiff_header, result);
    }

    TEST_METHOD(invalid_width) // NOLINT
    {
        spiff_header spiff_header{create_valid_spiff_header()};
        constexpr frame_info frame_info{create_valid_frame_info()};
        spiff_header.width = 27;

        const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        Assert::AreEqual(jpegls_errc::invalid_spiff_header, result);
    }

    TEST_METHOD(invalid_color_space) // NOLINT
    {
        spiff_header spiff_header{create_valid_spiff_header()};
        constexpr frame_info frame_info{create_valid_frame_info()};
        spiff_header.color_space = static_cast<spiff_color_space>(27);

        auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        Assert::IsTrue(result == jpegls_errc::invalid_spiff_header);

        spiff_header.color_space = spiff_color_space::bi_level_black;
        result = charls_validate_spiff_header(&spiff_header, &frame_info);
        Assert::AreEqual(jpegls_errc::invalid_spiff_header, result);
    }

    TEST_METHOD(invalid_color_space_component_count) // NOLINT
    {
        spiff_header spiff_header{create_valid_spiff_header()};
        constexpr frame_info frame_info{create_valid_frame_info()};
        spiff_header.color_space = spiff_color_space::grayscale;

        auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        Assert::AreEqual(jpegls_errc::invalid_spiff_header, result);

        spiff_header.color_space = spiff_color_space::cmyk;
        result = charls_validate_spiff_header(&spiff_header, &frame_info);
        Assert::AreEqual(jpegls_errc::invalid_spiff_header, result);
    }

    TEST_METHOD(invalid_resolution_units) // NOLINT
    {
        spiff_header spiff_header{create_valid_spiff_header()};
        constexpr frame_info frame_info{create_valid_frame_info()};
        spiff_header.resolution_units = static_cast<spiff_resolution_units>(99);

        const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        Assert::AreEqual(jpegls_errc::invalid_spiff_header, result);
    }

    TEST_METHOD(invalid_vertical_resolution) // NOLINT
    {
        spiff_header spiff_header{create_valid_spiff_header()};
        constexpr frame_info frame_info{create_valid_frame_info()};
        spiff_header.vertical_resolution = 0;

        const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        Assert::AreEqual(jpegls_errc::invalid_spiff_header, result);
    }

    TEST_METHOD(invalid_horizontal_resolution) // NOLINT
    {
        spiff_header spiff_header{create_valid_spiff_header()};
        constexpr frame_info frame_info{create_valid_frame_info()};
        spiff_header.horizontal_resolution = 0;

        const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        Assert::AreEqual(jpegls_errc::invalid_spiff_header, result);
    }

    TEST_METHOD(spiff_header_nullptr) // NOLINT
    {
        constexpr frame_info frame_info{create_valid_frame_info()};

        const auto result{charls_validate_spiff_header(nullptr, &frame_info)};
        Assert::AreEqual(jpegls_errc::invalid_argument, result);
    }

    TEST_METHOD(frame_info_nullptr) // NOLINT
    {
        constexpr spiff_header spiff_header{create_valid_spiff_header()};

        const auto result{charls_validate_spiff_header(&spiff_header, nullptr)};
        Assert::AreEqual(jpegls_errc::invalid_argument, result);
    }
};

} // namespace charls::test

MSVC_WARNING_UNSUPPRESS()
