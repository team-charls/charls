// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "../src/default_traits.h"
#include "../src/lossless_traits.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace charls { namespace test {

TEST_CLASS(lossless_traits_test)
{
public:
    TEST_METHOD(test_traits_16_bit) // NOLINT
    {
        using lossless_traits = lossless_traits<uint16_t, 12>;
        const auto traits1{default_traits<uint16_t, uint16_t>(4095, 0)};
        const lossless_traits traits2{};

        Assert::IsTrue(traits1.limit == traits2.limit);
        Assert::IsTrue(traits1.maximum_sample_value == traits2.maximum_sample_value);
        Assert::IsTrue(traits1.reset_threshold == traits2.reset_threshold);
        Assert::IsTrue(traits1.bits_per_pixel == traits2.bits_per_pixel);
        Assert::IsTrue(traits1.quantized_bits_per_pixel == traits2.quantized_bits_per_pixel);

        for (int i{-4096}; i <= 4096; ++i)
        {
            Assert::IsTrue(traits1.modulo_range(i) == lossless_traits::modulo_range(i));
            Assert::IsTrue(traits1.compute_error_value(i) == lossless_traits::compute_error_value(i));
        }

        for (int i{-8095}; i <= 8095; ++i)
        {
            Assert::IsTrue(traits1.correct_prediction(i) == lossless_traits::correct_prediction(i));
            Assert::IsTrue(traits1.is_near(i, 2) == lossless_traits::is_near(i, 2));
        }
    }

    TEST_METHOD(test_traits_8_bit) // NOLINT
    {
        using lossless_traits = lossless_traits<uint8_t, 8>;
        const auto traits1{default_traits<uint8_t, uint8_t>(255, 0)};
        const lossless_traits traits2{};

        Assert::IsTrue(traits1.limit == traits2.limit);
        Assert::IsTrue(traits1.maximum_sample_value == traits2.maximum_sample_value);
        Assert::IsTrue(traits1.reset_threshold == traits2.reset_threshold);
        Assert::IsTrue(traits1.bits_per_pixel == traits2.bits_per_pixel);
        Assert::IsTrue(traits1.quantized_bits_per_pixel == traits2.quantized_bits_per_pixel);

        for (int i{-255}; i <= 255; ++i)
        {
            Assert::IsTrue(traits1.modulo_range(i) == lossless_traits::modulo_range(i));
            Assert::IsTrue(traits1.compute_error_value(i) == lossless_traits::compute_error_value(i));
        }

        for (int i{-255}; i <= 512; ++i)
        {
            Assert::IsTrue(traits1.correct_prediction(i) == lossless_traits::correct_prediction(i));
            Assert::IsTrue(traits1.is_near(i, 2) == lossless_traits::is_near(i, 2));
        }
    }
};

}} // namespace charls::test
