// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "support.hpp"

#include "../src/color_transform.hpp"

#include <cstdint>

namespace charls::test {

TEST(color_transform_test, transform_hp1_round_trip)
{
    // For the normal unit test keep the range small for a quick test.
    // For a complete test which will take a while set the start and end to 0 and 256.
    constexpr int start_value{123};
    constexpr int end_value{124};

    for (int red{start_value}; red != end_value; ++red)
    {
        for (int green{}; green != 256; ++green)
        {
            for (int blue{}; blue != 256; ++blue)
            {
                constexpr transform_hp1<uint8_t> transform{};
                const auto sample{transform(red, green, blue)};
                constexpr transform_hp1<uint8_t>::inverse inverse{};

                const auto round_trip{inverse(sample.v1, sample.v2, sample.v3)};

                ASSERT_EQ(static_cast<uint8_t>(red), round_trip.v1);
                ASSERT_EQ(static_cast<uint8_t>(green), round_trip.v2);
                ASSERT_EQ(static_cast<uint8_t>(blue), round_trip.v3);
            }
        }
    }
}

TEST(color_transform_test, transform_hp2_round_trip)
{
    // For the normal unit test keep the range small for a quick test.
    // For a complete test which will take a while set the start and end to 0 and 256.
    constexpr int start_value{123};
    constexpr int end_value{124};

    for (int red{start_value}; red != end_value; ++red)
    {
        for (int green{}; green != 256; ++green)
        {
            for (int blue{}; blue != 256; ++blue)
            {
                constexpr transform_hp2<uint8_t> transform{};
                const auto sample{transform(red, green, blue)};
                constexpr transform_hp2<uint8_t>::inverse inverse{};

                const auto round_trip{inverse(sample.v1, sample.v2, sample.v3)};

                ASSERT_EQ(static_cast<uint8_t>(red), round_trip.v1);
                ASSERT_EQ(static_cast<uint8_t>(green), round_trip.v2);
                ASSERT_EQ(static_cast<uint8_t>(blue), round_trip.v3);
            }
        }
    }
}

TEST(color_transform_test, transform_hp2_neutral_grey)
{
    constexpr int red{128};
    constexpr int green{128};
    constexpr int blue{128};

    constexpr transform_hp2<uint8_t> transform{};
    const auto sample{transform(red, green, blue)};

    ASSERT_EQ(static_cast<uint8_t>(red), sample.v1);
    ASSERT_EQ(static_cast<uint8_t>(green), sample.v2);
    ASSERT_EQ(static_cast<uint8_t>(blue), sample.v3);
}

TEST(color_transform_test, transform_hp2_round_trip_near_lossless_8)
{
    constexpr int red{};
    constexpr int green{};
    constexpr int blue{};

    constexpr transform_hp2<uint8_t> transform{};
    auto sample{transform(red, green, blue)};

    sample.v1 -= 8;

    // The color transformation requires that the encoding is lossless, otherwise they will fail.
    // Clamping the values, would make some transformations work, but cause other transformations to fail.
    constexpr transform_hp2<uint8_t>::inverse inverse{};
    const auto round_trip{inverse(sample.v1, sample.v2, sample.v3)};
    ASSERT_NE(static_cast<uint8_t>(red), round_trip.v1);
    ASSERT_EQ(static_cast<uint8_t>(green), round_trip.v2);
    ASSERT_NE(static_cast<uint8_t>(blue), round_trip.v3);
}

TEST(color_transform_test, transform_hp3_round_trip)
{
    // For the normal unit test keep the range small for a quick test.
    // For a complete test which will take a while set the start and end to 0 and 256.
    constexpr int start_value{123};
    constexpr int end_value{124};

    for (int red{start_value}; red != end_value; ++red)
    {
        for (int green{}; green != 256; ++green)
        {
            for (int blue{}; blue != 256; ++blue)
            {
                constexpr transform_hp3<uint8_t> transformation{};
                const auto sample{transformation(red, green, blue)};
                constexpr transform_hp3<uint8_t>::inverse inverse{};
                const auto round_trip{inverse(sample.v1, sample.v2, sample.v3)};

                ASSERT_EQ(static_cast<uint8_t>(red), round_trip.v1);
                ASSERT_EQ(static_cast<uint8_t>(green), round_trip.v2);
                ASSERT_EQ(static_cast<uint8_t>(blue), round_trip.v3);
            }
        }
    }
}

TEST(color_transform_test, transform_hp3_sample_values_from_appendix_f)
{
    // ISO/IEC 14495-2:2003 appendix F.2: contains a sample transformation of the RCT
    constexpr int red{200};
    constexpr int green{10};
    constexpr int blue{55};

    constexpr transform_hp3<uint8_t> transform{};
    const auto sample{transform(red, green, blue)};

    ASSERT_EQ(static_cast<uint8_t>(4), sample.v1);
    ASSERT_EQ(static_cast<uint8_t>(173), sample.v2);
    ASSERT_EQ(static_cast<uint8_t>(62), sample.v3);

    constexpr transform_hp3<uint8_t>::inverse inverse{};
    const auto round_trip{inverse(sample.v1, sample.v2, sample.v3)};
    ASSERT_EQ(static_cast<uint8_t>(red), round_trip.v1);
    ASSERT_EQ(static_cast<uint8_t>(green), round_trip.v2);
    ASSERT_EQ(static_cast<uint8_t>(blue), round_trip.v3);
}

TEST(color_transform_test, hp_transformation_sample_images)
{
    decode_encode_file("data/banny.jls", "data/banny.ppm", false);
    decode_encode_file("data/banny-hp1.jls", "data/banny.ppm", false);
    decode_encode_file("data/banny-hp2.jls", "data/banny.ppm", false);
    decode_encode_file("data/banny-hp3.jls", "data/banny.ppm", false);
}

} // namespace charls::test
