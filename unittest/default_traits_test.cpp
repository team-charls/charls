// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "../src/default_traits.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace charls { namespace test {

TEST_CLASS(default_traits_test)
{
public:
    TEST_METHOD(create) // NOLINT
    {
        const default_traits<uint8_t, uint8_t> traits((1 << 8) - 1, 0);

        Assert::AreEqual(255, traits.maximum_sample_value);
        Assert::AreEqual(256, traits.range);
        Assert::AreEqual(0, traits.near_lossless);
        Assert::AreEqual(8, traits.quantized_bits_per_pixel);
        Assert::AreEqual(8, traits.bits_per_pixel);
        Assert::AreEqual(32, traits.limit);
        Assert::AreEqual(64, traits.reset_threshold);
    }

    TEST_METHOD(modulo_range) // NOLINT
    {
        const default_traits<uint8_t, uint8_t> traits(24, 0);

        for (int i = -25; i < 26; ++i)
        {
            const auto error_value = traits.modulo_range(i);
            constexpr int range = 24 + 1;
            Assert::IsTrue(-range / 2 <= error_value && error_value <= ((range + 1) / 2) - 1);
        }
    }
};

}} // namespace charls::test
