// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "../src/default_traits.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace CharLSUnitTest {

// clang-format off

TEST_CLASS(DefaultTraitsTest)
{
public:
    TEST_METHOD(Create)
    {
        const charls::DefaultTraits<uint8_t, uint8_t> traits((1 << 8) - 1, 0);

        Assert::AreEqual(255, traits.MAXVAL);
        Assert::AreEqual(256, traits.RANGE);
        Assert::AreEqual(0, traits.NEAR);
        Assert::AreEqual(8, traits.qbpp);
        Assert::AreEqual(8, traits.bpp);
        Assert::AreEqual(32, traits.LIMIT);
        Assert::AreEqual(64, traits.RESET);
    }

    TEST_METHOD(ModuloRange)
    {
        const charls::DefaultTraits<uint8_t, uint8_t> traits(24, 0);

        for (int i = -25; i < 26; ++i)
        {
            const auto error_value = traits.ModuloRange(i);
            constexpr int range = 24 + 1;
            Assert::IsTrue(-range / 2 <= error_value && error_value <= ((range + 1) / 2) - 1);
        }
    }
};

}
