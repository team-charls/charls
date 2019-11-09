// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "../src/color_transform.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace CharLSUnitTest {

// clang-format off

TEST_CLASS(ColorTransformTest)
{
public:
    TEST_METHOD(TransformHp1RoundTrip)
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
                    const charls::TransformHp1<uint8_t> transform;
                    const auto sample = transform(red, green, blue);
                    const charls::TransformHp1<uint8_t>::Inverse inverse(transform);

                    const auto roundTrip = inverse(sample.v1, sample.v2, sample.v3);

                    Assert::AreEqual(static_cast<uint8_t>(red), roundTrip.R);
                    Assert::AreEqual(static_cast<uint8_t>(green), roundTrip.G);
                    Assert::AreEqual(static_cast<uint8_t>(blue), roundTrip.B);
                }
            }
        }
    }

    TEST_METHOD(TransformHp2RoundTrip)
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
                    const charls::TransformHp2<uint8_t> transform;
                    const auto sample = transform(red, green, blue);
                    const charls::TransformHp2<uint8_t>::Inverse inverse(transform);

                    const auto roundTrip = inverse(sample.v1, sample.v2, sample.v3);

                    Assert::AreEqual(static_cast<uint8_t>(red), roundTrip.R);
                    Assert::AreEqual(static_cast<uint8_t>(green), roundTrip.G);
                    Assert::AreEqual(static_cast<uint8_t>(blue), roundTrip.B);
                }
            }
        }
    }

    TEST_METHOD(TransformHp3RoundTrip)
    {
        // For the normal unit test keep the range small for a quick test.
        // For a complete test which will take a while set the start and end to 0 and 255.
        constexpr uint8_t startValue = 123;
        constexpr uint8_t endValue = 124;

        const charls::TransformHp3<uint8_t> transformation;

        for (int red = startValue; red < endValue; ++red)
        {
            for (int green = 0; green < 255; ++green)
            {
                for (int blue = 0; blue < 255; ++blue)
                {
                    const auto sample = transformation(red, green, blue);
                    const charls::TransformHp3<uint8_t>::Inverse inverse(transformation);
                    const auto roundTrip = inverse(sample.v1, sample.v2, sample.v3);

                    Assert::AreEqual(static_cast<uint8_t>(red), roundTrip.R);
                    Assert::AreEqual(static_cast<uint8_t>(green), roundTrip.G);
                    Assert::AreEqual(static_cast<uint8_t>(blue), roundTrip.B);
                }
            }
        }
    }
};

}
