// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "../src/color_transform.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace charls {
namespace test {

// clang-format off

TEST_CLASS(color_transform_test)
{
public:
    TEST_METHOD(transform_hp1_round_trip) // NOLINT
    {
        // For the normal unit test keep the range small for a quick test.
        // For a complete test which will take a while set the start and end to 0 and 255.
        constexpr int startValue = 123;
        constexpr int endValue = 124;

        for (int red = startValue; red < endValue; ++red)
        {
            for (int green = 0; green < 255; ++green)
            {
                for (int blue = 0; blue < 255; ++blue)
                {
                    const charls::transform_hp1<uint8_t> transform;
                    const auto sample = transform(red, green, blue);
                    const charls::transform_hp1<uint8_t>::inverse inverse(transform);

                    const auto roundTrip = inverse(sample.v1, sample.v2, sample.v3);

                    Assert::AreEqual(static_cast<uint8_t>(red), roundTrip.R);
                    Assert::AreEqual(static_cast<uint8_t>(green), roundTrip.G);
                    Assert::AreEqual(static_cast<uint8_t>(blue), roundTrip.B);
                }
            }
        }
    }

    TEST_METHOD(transform_hp2_round_trip) // NOLINT
    {
        // For the normal unit test keep the range small for a quick test.
        // For a complete test which will take a while set the start and end to 0 and 255.
        constexpr int startValue = 123;
        constexpr int endValue = 124;

        for (int red = startValue; red < endValue; ++red)
        {
            for (int green = 0; green < 255; ++green)
            {
                for (int blue = 0; blue < 255; ++blue)
                {
                    const charls::transform_hp2<uint8_t> transform;
                    const auto sample = transform(red, green, blue);
                    const charls::transform_hp2<uint8_t>::inverse inverse(transform);

                    const auto roundTrip = inverse(sample.v1, sample.v2, sample.v3);

                    Assert::AreEqual(static_cast<uint8_t>(red), roundTrip.R);
                    Assert::AreEqual(static_cast<uint8_t>(green), roundTrip.G);
                    Assert::AreEqual(static_cast<uint8_t>(blue), roundTrip.B);
                }
            }
        }
    }

    TEST_METHOD(transform_hp3_round_trip) // NOLINT
    {
        // For the normal unit test keep the range small for a quick test.
        // For a complete test which will take a while set the start and end to 0 and 255.
        constexpr uint8_t startValue = 123;
        constexpr uint8_t endValue = 124;

        const charls::transform_hp3<uint8_t> transformation;

        for (int red = startValue; red < endValue; ++red)
        {
            for (int green = 0; green < 255; ++green)
            {
                for (int blue = 0; blue < 255; ++blue)
                {
                    const auto sample = transformation(red, green, blue);
                    const charls::transform_hp3<uint8_t>::inverse inverse(transformation);
                    const auto roundTrip = inverse(sample.v1, sample.v2, sample.v3);

                    Assert::AreEqual(static_cast<uint8_t>(red), roundTrip.R);
                    Assert::AreEqual(static_cast<uint8_t>(green), roundTrip.G);
                    Assert::AreEqual(static_cast<uint8_t>(blue), roundTrip.B);
                }
            }
        }
    }
};

} // namespace test
} // namespace charls
