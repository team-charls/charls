// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include "../src/color_transform.h"

#include <charls/charls.h>

#include <tuple>
#include <vector>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::byte;
using std::vector;

namespace charls::test {

TEST_CLASS(color_transform_test)
{
public:
    TEST_METHOD(transform_hp1_round_trip) // NOLINT
    {
        // For the normal unit test keep the range small for a quick test.
        // For a complete test which will take a while set the start and end to 0 and 255.
        constexpr int start_value{123};
        constexpr int end_value{124};

        for (int red{start_value}; red != end_value; ++red)
        {
            for (int green{}; green != 255; ++green)
            {
                for (int blue{}; blue != 255; ++blue)
                {
                    constexpr transform_hp1<uint8_t> transform{};
                    const auto sample{transform(red, green, blue)};
                    const transform_hp1<uint8_t>::inverse inverse(transform);

                    const auto round_trip{inverse(sample.v1, sample.v2, sample.v3)};

                    Assert::AreEqual(static_cast<uint8_t>(red), round_trip.R);
                    Assert::AreEqual(static_cast<uint8_t>(green), round_trip.G);
                    Assert::AreEqual(static_cast<uint8_t>(blue), round_trip.B);
                }
            }
        }
    }

    TEST_METHOD(transform_hp2_round_trip) // NOLINT
    {
        // For the normal unit test keep the range small for a quick test.
        // For a complete test which will take a while set the start and end to 0 and 255.
        constexpr int start_value{123};
        constexpr int end_value{124};

        for (int red{start_value}; red != end_value; ++red)
        {
            for (int green{}; green != 255; ++green)
            {
                for (int blue{}; blue != 255; ++blue)
                {
                    constexpr transform_hp2<uint8_t> transform{};
                    const auto sample{transform(red, green, blue)};
                    const transform_hp2<uint8_t>::inverse inverse(transform);

                    const auto round_trip{inverse(sample.v1, sample.v2, sample.v3)};

                    Assert::AreEqual(static_cast<uint8_t>(red), round_trip.R);
                    Assert::AreEqual(static_cast<uint8_t>(green), round_trip.G);
                    Assert::AreEqual(static_cast<uint8_t>(blue), round_trip.B);
                }
            }
        }
    }

    TEST_METHOD(transform_hp3_round_trip) // NOLINT
    {
        // For the normal unit test keep the range small for a quick test.
        // For a complete test which will take a while set the start and end to 0 and 255.
        constexpr uint8_t start_value{123};
        constexpr uint8_t end_value{124};

        for (int red{start_value}; red != end_value; ++red)
        {
            for (int green{}; green != 255; ++green)
            {
                for (int blue{}; blue != 255; ++blue)
                {
                    constexpr transform_hp3<uint8_t> transformation{};
                    const auto sample{transformation(red, green, blue)};
                    const transform_hp3<uint8_t>::inverse inverse(transformation);
                    const auto round_trip{inverse(sample.v1, sample.v2, sample.v3)};

                    Assert::AreEqual(static_cast<uint8_t>(red), round_trip.R);
                    Assert::AreEqual(static_cast<uint8_t>(green), round_trip.G);
                    Assert::AreEqual(static_cast<uint8_t>(blue), round_trip.B);
                }
            }
        }
    }

    TEST_METHOD(decode_non_8_or_16_bit_that_is_not_supported_throws) // NOLINT
    {
        const auto jpegls_data{read_file("land10-10bit-rgb-hp3-invalid.jls")};

        const jpegls_decoder decoder{jpegls_data, true};

        vector<byte> destination(decoder.destination_size());

        assert_expect_exception(jpegls_errc::bit_depth_for_transform_not_supported,
                                [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(encode_non_8_or_16_bit_that_is_not_supported_throws) // NOLINT
    {
        constexpr frame_info frame_info{2, 1, 10, 3};
        jpegls_encoder encoder;

        vector<byte> destination(40);
        encoder.destination(destination).frame_info(frame_info).color_transformation(color_transformation::hp3);
        const vector<byte> source(20);
        assert_expect_exception(jpegls_errc::bit_depth_for_transform_not_supported,
                                [&encoder, &source] { std::ignore = encoder.encode(source); });
    }
};

} // namespace charls::test
