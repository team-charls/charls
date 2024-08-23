// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "jpegls_preset_coding_parameters_test.hpp"

#include "../src/jpegls_preset_coding_parameters.hpp"

#include <limits>


using Microsoft::VisualStudio::CppUnitTestFramework::Assert;


namespace charls::test {

TEST_CLASS(jpegls_preset_coding_parameters_test)
{
public:
    TEST_METHOD(table_c3) // NOLINT
    {
        const auto parameters{compute_default(255, 0)};

        Assert::AreEqual(255, parameters.maximum_sample_value);
        Assert::AreEqual(3, parameters.threshold1);
        Assert::AreEqual(7, parameters.threshold2);
        Assert::AreEqual(21, parameters.threshold3);
        Assert::AreEqual(64, parameters.reset_value);
    }

    TEST_METHOD(max_value_lossless) // NOLINT
    {
        constexpr auto expected{compute_defaults_using_reference_implementation(std::numeric_limits<uint16_t>::max(), 0)};
        const auto parameters{compute_default(65535, 0)};

        Assert::AreEqual(expected.max_value, parameters.maximum_sample_value);
        Assert::AreEqual(expected.t1, parameters.threshold1);
        Assert::AreEqual(expected.t2, parameters.threshold2);
        Assert::AreEqual(expected.t3, parameters.threshold3);
        Assert::AreEqual(expected.reset, parameters.reset_value);
    }

    TEST_METHOD(min_value_lossless) // NOLINT
    {
        constexpr auto expected{compute_defaults_using_reference_implementation(3, 0)};
        const auto parameters{compute_default(3, 0)};

        Assert::AreEqual(expected.max_value, parameters.maximum_sample_value);
        Assert::AreEqual(expected.t1, parameters.threshold1);
        Assert::AreEqual(expected.t2, parameters.threshold2);
        Assert::AreEqual(expected.t3, parameters.threshold3);
        Assert::AreEqual(expected.reset, parameters.reset_value);
    }

    TEST_METHOD(min_high_value_lossless) // NOLINT
    {
        constexpr auto expected{compute_defaults_using_reference_implementation(128, 0)};
        const auto parameters{compute_default(128, 0)};

        Assert::AreEqual(expected.max_value, parameters.maximum_sample_value);
        Assert::AreEqual(expected.t1, parameters.threshold1);
        Assert::AreEqual(expected.t2, parameters.threshold2);
        Assert::AreEqual(expected.t3, parameters.threshold3);
        Assert::AreEqual(expected.reset, parameters.reset_value);
    }

    TEST_METHOD(max_low_value_lossless) // NOLINT
    {
        constexpr auto expected{compute_defaults_using_reference_implementation(127, 0)};
        const auto parameters{compute_default(127, 0)};

        Assert::AreEqual(expected.max_value, parameters.maximum_sample_value);
        Assert::AreEqual(expected.t1, parameters.threshold1);
        Assert::AreEqual(expected.t2, parameters.threshold2);
        Assert::AreEqual(expected.t3, parameters.threshold3);
        Assert::AreEqual(expected.reset, parameters.reset_value);
    }

    TEST_METHOD(max_value_max_lossy) // NOLINT
    {
        constexpr auto expected{compute_defaults_using_reference_implementation(std::numeric_limits<uint16_t>::max(), 255)};
        const auto parameters{compute_default(65535, 255)};

        Assert::AreEqual(expected.max_value, parameters.maximum_sample_value);
        Assert::AreEqual(expected.t1, parameters.threshold1);
        Assert::AreEqual(expected.t2, parameters.threshold2);
        Assert::AreEqual(expected.t3, parameters.threshold3);
        Assert::AreEqual(expected.reset, parameters.reset_value);
    }

    TEST_METHOD(min_value_max_lossy) // NOLINT
    {
        constexpr auto expected{compute_defaults_using_reference_implementation(3, 1)};
        const auto parameters{compute_default(3, 1)};

        Assert::AreEqual(expected.max_value, parameters.maximum_sample_value);
        Assert::AreEqual(expected.t1, parameters.threshold1);
        Assert::AreEqual(expected.t2, parameters.threshold2);
        Assert::AreEqual(expected.t3, parameters.threshold3);
        Assert::AreEqual(expected.reset, parameters.reset_value);
    }

    TEST_METHOD(is_valid_default) // NOLINT
    {
        constexpr auto bits_per_sample{16};
        constexpr auto maximum_component_value{(1 << bits_per_sample) - 1};
        constexpr jpegls_pc_parameters pc_parameters{};

        Assert::IsTrue(is_valid(pc_parameters, maximum_component_value, 0));
    }

    TEST_METHOD(is_valid_thresholds_zero) // NOLINT
    {
        constexpr auto bits_per_sample{16};
        constexpr auto maximum_component_value{(1 << bits_per_sample) - 1};
        constexpr jpegls_pc_parameters pc_parameters{maximum_component_value, 0, 0, 0, 63};

        Assert::IsTrue(is_valid(pc_parameters, maximum_component_value, 0));
    }

    TEST_METHOD(is_default_nothing_set) // NOLINT
    {
        const auto default_parameters{compute_default(255, 0)};

        constexpr jpegls_pc_parameters pc_parameters{};

        Assert::IsTrue(is_default(pc_parameters, default_parameters));
    }

    TEST_METHOD(is_default_same_as_default) // NOLINT
    {
        const auto default_parameters{compute_default(255, 0)};

        const jpegls_pc_parameters pc_parameters{compute_default(255, 0)};

        Assert::IsTrue(is_default(pc_parameters, default_parameters));
    }

    TEST_METHOD(is_default_same_as_default_except_reset) // NOLINT
    {
        const auto default_parameters{compute_default(255, 0)};

        jpegls_pc_parameters pc_parameters{compute_default(255, 0)};
        ++pc_parameters.reset_value;

        Assert::IsFalse(is_default(pc_parameters, default_parameters));
    }
};

} // namespace charls::test
