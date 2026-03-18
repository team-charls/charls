// SPDX-FileCopyrightText: © 2022 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>

#include "support.hpp"

#include <charls/validate_spiff_header.h>

// TODO: enable
// MSVC_WARNING_SUPPRESS(6387) // '_Param_(x)' could be '0': this does not adhere to the specification for the function.

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

TEST(charls_validate_spiff_header_test, valid)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    constexpr frame_info frame_info{create_valid_frame_info()};

    auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::success, result);

    spiff_header.color_space = spiff_color_space::none;
    result = charls_validate_spiff_header(&spiff_header, &frame_info);
    EXPECT_EQ(jpegls_errc::success, result);
}

TEST(charls_validate_spiff_header_test, invalid_compression_type)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    constexpr frame_info frame_info{create_valid_frame_info()};
    spiff_header.compression_type = spiff_compression_type::uncompressed;

    const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);
}

TEST(charls_validate_spiff_header_test, invalid_profile_id)
{
    spiff_header spiff_header{};
    constexpr frame_info frame_info{};
    spiff_header.compression_type = spiff_compression_type::jpeg_ls;
    spiff_header.profile_id = spiff_profile_id::continuous_tone_base;

    const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);
}

TEST(charls_validate_spiff_header_test, invalid_component_count)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    constexpr frame_info frame_info{create_valid_frame_info()};
    spiff_header.component_count = 7;

    const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);
}

TEST(charls_validate_spiff_header_test, all_jpegls_bits_per_sample_are_valid)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    frame_info frame_info{create_valid_frame_info()};

    for (int bits_per_sample{2}; bits_per_sample <= 16; ++bits_per_sample)
    {
        spiff_header.bits_per_sample = bits_per_sample;
        frame_info.bits_per_sample = bits_per_sample;

        const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
        EXPECT_EQ(jpegls_errc::success, result);
    }
}

TEST(charls_validate_spiff_header_test, invalid_bits_per_sample)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    constexpr frame_info frame_info{create_valid_frame_info()};
    spiff_header.bits_per_sample = 12;

    const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);
}

TEST(charls_validate_spiff_header_test, invalid_height)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    constexpr frame_info frame_info{create_valid_frame_info()};
    spiff_header.height = 333;

    const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);
}

TEST(charls_validate_spiff_header_test, invalid_width)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    constexpr frame_info frame_info{create_valid_frame_info()};
    spiff_header.width = 27;

    const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);
}

TEST(charls_validate_spiff_header_test, invalid_color_space)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    constexpr frame_info frame_info{create_valid_frame_info()};
    spiff_header.color_space = static_cast<spiff_color_space>(27);

    auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);

    spiff_header.color_space = spiff_color_space::bi_level_black;
    result = charls_validate_spiff_header(&spiff_header, &frame_info);
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);
}

TEST(charls_validate_spiff_header_test, valid_color_space_component_count)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    frame_info frame_info{create_valid_frame_info()};

    frame_info.component_count = 1;
    spiff_header.component_count = 1;
    spiff_header.color_space = spiff_color_space::grayscale;
    auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::success, result);

    frame_info.component_count = 3;
    spiff_header.component_count = 3;
    spiff_header.color_space = spiff_color_space::ycbcr_itu_bt_601_1_video;
    result = charls_validate_spiff_header(&spiff_header, &frame_info);
    EXPECT_EQ(jpegls_errc::success, result);

    frame_info.component_count = 4;
    spiff_header.component_count = 4;
    spiff_header.color_space = spiff_color_space::cmyk;
    result = charls_validate_spiff_header(&spiff_header, &frame_info);
    EXPECT_EQ(jpegls_errc::success, result);
}

TEST(charls_validate_spiff_header_test, invalid_color_space_component_count)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    frame_info frame_info{create_valid_frame_info()};
    spiff_header.color_space = spiff_color_space::grayscale;

    auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);

    spiff_header.color_space = spiff_color_space::cmyk;
    result = charls_validate_spiff_header(&spiff_header, &frame_info);
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);

    frame_info.component_count = 1;
    spiff_header.component_count = 1;
    spiff_header.color_space = spiff_color_space::rgb;
    result = charls_validate_spiff_header(&spiff_header, &frame_info);
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);
}

TEST(charls_validate_spiff_header_test, invalid_resolution_units)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    constexpr frame_info frame_info{create_valid_frame_info()};
    spiff_header.resolution_units = static_cast<spiff_resolution_units>(99);

    const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);
}

TEST(charls_validate_spiff_header_test, invalid_vertical_resolution)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    constexpr frame_info frame_info{create_valid_frame_info()};
    spiff_header.vertical_resolution = 0;

    const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);
}

TEST(charls_validate_spiff_header_test, invalid_horizontal_resolution)
{
    spiff_header spiff_header{create_valid_spiff_header()};
    constexpr frame_info frame_info{create_valid_frame_info()};
    spiff_header.horizontal_resolution = 0;

    const auto result{charls_validate_spiff_header(&spiff_header, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_spiff_header, result);
}

TEST(charls_validate_spiff_header_test, spiff_header_nullptr)
{
    constexpr frame_info frame_info{create_valid_frame_info()};

    const auto result{charls_validate_spiff_header(nullptr, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_argument, result);
}

TEST(charls_validate_spiff_header_test, frame_info_nullptr)
{
    constexpr spiff_header spiff_header{create_valid_spiff_header()};

    const auto result{charls_validate_spiff_header(&spiff_header, nullptr)};
    EXPECT_EQ(jpegls_errc::invalid_argument, result);
}

} // namespace charls::test

////MSVC_WARNING_UNSUPPRESS()
