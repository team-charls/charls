// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "util.hpp"

#include "../src/color_transform.hpp"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

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
                    constexpr transform_hp1<uint8_t>::inverse inverse{};

                    const auto round_trip{inverse(sample.v1, sample.v2, sample.v3)};

                    // Note: Is True is 1000 times faster then AreEqual
                    Assert::IsTrue(static_cast<uint8_t>(red) == round_trip.v1);
                    Assert::IsTrue(static_cast<uint8_t>(green) == round_trip.v2);
                    Assert::IsTrue(static_cast<uint8_t>(blue) == round_trip.v3);
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
                    constexpr transform_hp2<uint8_t>::inverse inverse{};

                    const auto round_trip{inverse(sample.v1, sample.v2, sample.v3)};

                    // Note: Is True is 1000 times faster then AreEqual
                    Assert::IsTrue(static_cast<uint8_t>(red) == round_trip.v1);
                    Assert::IsTrue(static_cast<uint8_t>(green) == round_trip.v2);
                    Assert::IsTrue(static_cast<uint8_t>(blue) == round_trip.v3);
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
                    constexpr transform_hp3<uint8_t>::inverse inverse{};
                    const auto round_trip{inverse(sample.v1, sample.v2, sample.v3)};

                    // Note: Is True is 1000 times faster then AreEqual
                    Assert::IsTrue(static_cast<uint8_t>(red) == round_trip.v1);
                    Assert::IsTrue(static_cast<uint8_t>(green) == round_trip.v2);
                    Assert::IsTrue(static_cast<uint8_t>(blue) == round_trip.v3);
                }
            }
        }
    }
};

} // namespace charls::test
