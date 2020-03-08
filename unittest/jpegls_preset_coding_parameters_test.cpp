// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "../src/jpegls_preset_coding_parameters.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace {

struct thresholds
{
    int32_t MaxVal;
    int32_t T1;
    int32_t T2;
    int32_t T3;
    int32_t Reset;
};

// Threshold function of JPEG-LS reference implementation.
constexpr thresholds ComputeDefaultsUsingReferenceImplementation(const int32_t max_value, const uint16_t near) noexcept
{
    thresholds result{max_value, 0, 0, 0, 64};

    if (result.MaxVal >= 128)
    {
        int32_t factor = result.MaxVal;
        if (factor > 4095)
            factor = 4095;
        factor = (factor + 128) >> 8;
        result.T1 = factor * (3 - 2) + 2 + 3 * near;
        if (result.T1 > result.MaxVal || result.T1 < near + 1)
            result.T1 = near + 1;
        result.T2 = factor * (7 - 3) + 3 + 5 * near;
        if (result.T2 > result.MaxVal || result.T2 < result.T1)
            result.T2 = result.T1;
        result.T3 = factor * (21 - 4) + 4 + 7 * near;
        if (result.T3 > result.MaxVal || result.T3 < result.T2)
            result.T3 = result.T2;
    }
    else
    {
        const int32_t factor = 256 / (result.MaxVal + 1);
        result.T1 = 3 / factor + 3 * near;
        if (result.T1 < 2)
            result.T1 = 2;
        if (result.T1 > result.MaxVal || result.T1 < near + 1)
            result.T1 = near + 1;
        result.T2 = 7 / factor + 5 * near;
        if (result.T2 < 3)
            result.T2 = 3;
        if (result.T2 > result.MaxVal || result.T2 < result.T1)
            result.T2 = result.T1;
        result.T3 = 21 / factor + 7 * near;
        if (result.T3 < 4)
            result.T3 = 4;
        if (result.T3 > result.MaxVal || result.T3 < result.T2)
            result.T3 = result.T2;
    }

    return result;
}

} // namespace

// clang-format off

namespace charls {
namespace test {

TEST_CLASS(JpeglsPresetCodingParametersTest)
{
public:
    TEST_METHOD(TableC3)
    {
        const auto parameters{compute_default(255, 0)};

        Assert::AreEqual(255, parameters.maximum_sample_value);
        Assert::AreEqual(3, parameters.threshold1);
        Assert::AreEqual(7, parameters.threshold2);
        Assert::AreEqual(21, parameters.threshold3);
        Assert::AreEqual(64, parameters.reset_value);
    }

    TEST_METHOD(MaxValueLossless)
    {
        const auto expected{ComputeDefaultsUsingReferenceImplementation(65535, 0)};
        const auto parameters{compute_default(65535, 0)};

        Assert::AreEqual(expected.MaxVal, parameters.maximum_sample_value);
        Assert::AreEqual(expected.T1, parameters.threshold1);
        Assert::AreEqual(expected.T2, parameters.threshold2);
        Assert::AreEqual(expected.T3, parameters.threshold3);
        Assert::AreEqual(expected.Reset, parameters.reset_value);
    }

    TEST_METHOD(MinValueLossless)
    {
        const auto expected{ComputeDefaultsUsingReferenceImplementation(3, 0)};
        const auto parameters{compute_default(3, 0)};

        Assert::AreEqual(expected.MaxVal, parameters.maximum_sample_value);
        Assert::AreEqual(expected.T1, parameters.threshold1);
        Assert::AreEqual(expected.T2, parameters.threshold2);
        Assert::AreEqual(expected.T3, parameters.threshold3);
        Assert::AreEqual(expected.Reset, parameters.reset_value);
    }

    TEST_METHOD(MinHighValueLossless)
    {
        const auto expected{ComputeDefaultsUsingReferenceImplementation(128, 0)};
        const auto parameters{compute_default(128, 0)};

        Assert::AreEqual(expected.MaxVal, parameters.maximum_sample_value);
        Assert::AreEqual(expected.T1, parameters.threshold1);
        Assert::AreEqual(expected.T2, parameters.threshold2);
        Assert::AreEqual(expected.T3, parameters.threshold3);
        Assert::AreEqual(expected.Reset, parameters.reset_value);
    }

    TEST_METHOD(MaxLowValueLossless)
    {
        const auto expected{ComputeDefaultsUsingReferenceImplementation(127, 0)};
        const auto parameters{compute_default(127, 0)};

        Assert::AreEqual(expected.MaxVal, parameters.maximum_sample_value);
        Assert::AreEqual(expected.T1, parameters.threshold1);
        Assert::AreEqual(expected.T2, parameters.threshold2);
        Assert::AreEqual(expected.T3, parameters.threshold3);
        Assert::AreEqual(expected.Reset, parameters.reset_value);
    }

    TEST_METHOD(MaxValueMaxLossy)
    {
        const auto expected{ComputeDefaultsUsingReferenceImplementation(65535, 255)};
        const auto parameters{compute_default(65535, 255)};

        Assert::AreEqual(expected.MaxVal, parameters.maximum_sample_value);
        Assert::AreEqual(expected.T1, parameters.threshold1);
        Assert::AreEqual(expected.T2, parameters.threshold2);
        Assert::AreEqual(expected.T3, parameters.threshold3);
        Assert::AreEqual(expected.Reset, parameters.reset_value);
    }

    TEST_METHOD(MinValueMaxLossy)
    {
        const auto expected{ComputeDefaultsUsingReferenceImplementation(3, 1)};
        const auto parameters{compute_default(3, 1)};

        Assert::AreEqual(expected.MaxVal, parameters.maximum_sample_value);
        Assert::AreEqual(expected.T1, parameters.threshold1);
        Assert::AreEqual(expected.T2, parameters.threshold2);
        Assert::AreEqual(expected.T3, parameters.threshold3);
        Assert::AreEqual(expected.Reset, parameters.reset_value);
    }

    TEST_METHOD(is_valid_default)
    {
        constexpr auto bits_per_sample = 16;
        constexpr auto maximum_component_value = (1 << bits_per_sample) - 1;
        const jpegls_pc_parameters pc_parameters{};

        Assert::IsTrue(is_valid(pc_parameters, maximum_component_value, 0));
    }

    TEST_METHOD(is_valid_thresholds_zero)
    {
        constexpr auto bits_per_sample = 16;
        constexpr auto maximum_component_value = (1 << bits_per_sample) - 1;
        const jpegls_pc_parameters pc_parameters{maximum_component_value, 0, 0, 0, 63};

        Assert::IsTrue(is_valid(pc_parameters, maximum_component_value, 0));
    }
};

}
}
