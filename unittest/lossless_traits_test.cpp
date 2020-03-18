// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "../src/default_traits.h"
#include "../src/lossless_traits.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace charls {
namespace test {

// clang-format off

TEST_CLASS(lossless_traits_test)
{
public:
    TEST_METHOD(TestTraits16bit) // NOLINT
    {
        using lossless_traits = LosslessTraits<uint16_t, 12>;
        const auto traits1 = DefaultTraits<uint16_t, uint16_t>(4095,0);
        const lossless_traits traits2;

        Assert::IsTrue(traits1.LIMIT == traits2.LIMIT);
        Assert::IsTrue(traits1.MAXVAL == traits2.MAXVAL);
        Assert::IsTrue(traits1.RESET == traits2.RESET);
        Assert::IsTrue(traits1.bpp == traits2.bpp);
        Assert::IsTrue(traits1.qbpp == traits2.qbpp);

        for (int i = -4096; i <= 4096; ++i)
        {
            Assert::IsTrue(traits1.ModuloRange(i) == lossless_traits::ModuloRange(i));
            Assert::IsTrue(traits1.ComputeErrVal(i) == lossless_traits::ComputeErrVal(i));
        }

        for (int i = -8095; i <= 8095; ++i)
        {
            Assert::IsTrue(traits1.CorrectPrediction(i) == lossless_traits::CorrectPrediction(i));
            Assert::IsTrue(traits1.IsNear(i,2) == lossless_traits::IsNear(i, 2));
        }
    }

    TEST_METHOD(TestTraits8bit) // NOLINT
    {
        using lossless_traits = LosslessTraits<uint8_t, 8>;
        const auto traits1 = DefaultTraits<uint8_t, uint8_t>(255,0);
        const lossless_traits traits2;

        Assert::IsTrue(traits1.LIMIT == traits2.LIMIT);
        Assert::IsTrue(traits1.MAXVAL == traits2.MAXVAL);
        Assert::IsTrue(traits1.RESET == traits2.RESET);
        Assert::IsTrue(traits1.bpp == traits2.bpp);
        Assert::IsTrue(traits1.qbpp == traits2.qbpp);

        for (int i = -255; i <= 255; ++i)
        {
            Assert::IsTrue(traits1.ModuloRange(i) == lossless_traits::ModuloRange(i));
            Assert::IsTrue(traits1.ComputeErrVal(i) == lossless_traits::ComputeErrVal(i));
        }

        for (int i = -255; i <= 512; ++i)
        {
            Assert::IsTrue(traits1.CorrectPrediction(i) == lossless_traits::CorrectPrediction(i));
            Assert::IsTrue(traits1.IsNear(i,2) == lossless_traits::IsNear(i,2));
        }
    }
};

} // namespace test
} // namespace charls
