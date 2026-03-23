// SPDX-FileCopyrightText: © 2019 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "../src/jpegls_preset_coding_parameters.hpp"

#include "jpegls_preset_coding_parameters_test.hpp"

#include <limits>


namespace charls::test {

TEST(jpegls_preset_coding_parameters_test, table_c3)
{
    const auto parameters{compute_default(255, 0)};

    EXPECT_EQ(255, parameters.maximum_sample_value);
    EXPECT_EQ(3, parameters.threshold1);
    EXPECT_EQ(7, parameters.threshold2);
    EXPECT_EQ(21, parameters.threshold3);
    EXPECT_EQ(64, parameters.reset_value);
}

TEST(jpegls_preset_coding_parameters_test, max_value_lossless)
{
    constexpr auto expected{compute_defaults_using_reference_implementation(std::numeric_limits<uint16_t>::max(), 0)};
    const auto parameters{compute_default(65535, 0)};

    EXPECT_EQ(expected.max_value, parameters.maximum_sample_value);
    EXPECT_EQ(expected.t1, parameters.threshold1);
    EXPECT_EQ(expected.t2, parameters.threshold2);
    EXPECT_EQ(expected.t3, parameters.threshold3);
    EXPECT_EQ(expected.reset, parameters.reset_value);
}

TEST(jpegls_preset_coding_parameters_test, min_value_lossless)
{
    constexpr auto expected{compute_defaults_using_reference_implementation(3, 0)};
    const auto parameters{compute_default(3, 0)};

    EXPECT_EQ(expected.max_value, parameters.maximum_sample_value);
    EXPECT_EQ(expected.t1, parameters.threshold1);
    EXPECT_EQ(expected.t2, parameters.threshold2);
    EXPECT_EQ(expected.t3, parameters.threshold3);
    EXPECT_EQ(expected.reset, parameters.reset_value);
}

TEST(jpegls_preset_coding_parameters_test, min_high_value_lossless)
{
    constexpr auto expected{compute_defaults_using_reference_implementation(128, 0)};
    const auto parameters{compute_default(128, 0)};

    EXPECT_EQ(expected.max_value, parameters.maximum_sample_value);
    EXPECT_EQ(expected.t1, parameters.threshold1);
    EXPECT_EQ(expected.t2, parameters.threshold2);
    EXPECT_EQ(expected.t3, parameters.threshold3);
    EXPECT_EQ(expected.reset, parameters.reset_value);
}

TEST(jpegls_preset_coding_parameters_test, max_low_value_lossless)
{
    constexpr auto expected{compute_defaults_using_reference_implementation(127, 0)};
    const auto parameters{compute_default(127, 0)};

    EXPECT_EQ(expected.max_value, parameters.maximum_sample_value);
    EXPECT_EQ(expected.t1, parameters.threshold1);
    EXPECT_EQ(expected.t2, parameters.threshold2);
    EXPECT_EQ(expected.t3, parameters.threshold3);
    EXPECT_EQ(expected.reset, parameters.reset_value);
}

TEST(jpegls_preset_coding_parameters_test, max_value_max_lossy)
{
    constexpr auto expected{compute_defaults_using_reference_implementation(std::numeric_limits<uint16_t>::max(), 255)};
    const auto parameters{compute_default(65535, 255)};

    EXPECT_EQ(expected.max_value, parameters.maximum_sample_value);
    EXPECT_EQ(expected.t1, parameters.threshold1);
    EXPECT_EQ(expected.t2, parameters.threshold2);
    EXPECT_EQ(expected.t3, parameters.threshold3);
    EXPECT_EQ(expected.reset, parameters.reset_value);
}

TEST(jpegls_preset_coding_parameters_test, min_value_max_lossy)
{
    constexpr auto expected{compute_defaults_using_reference_implementation(3, 1)};
    const auto parameters{compute_default(3, 1)};

    EXPECT_EQ(expected.max_value, parameters.maximum_sample_value);
    EXPECT_EQ(expected.t1, parameters.threshold1);
    EXPECT_EQ(expected.t2, parameters.threshold2);
    EXPECT_EQ(expected.t3, parameters.threshold3);
    EXPECT_EQ(expected.reset, parameters.reset_value);
}

TEST(jpegls_preset_coding_parameters_test, is_valid_default)
{
    constexpr auto bits_per_sample{16};
    constexpr auto maximum_component_value{(1 << bits_per_sample) - 1};
    constexpr jpegls_pc_parameters pc_parameters{};

    EXPECT_TRUE(is_valid(pc_parameters, maximum_component_value, 0));
}

TEST(jpegls_preset_coding_parameters_test, is_valid_thresholds_zero)
{
    constexpr auto bits_per_sample{16};
    constexpr auto maximum_component_value{(1 << bits_per_sample) - 1};
    constexpr jpegls_pc_parameters pc_parameters{maximum_component_value, 0, 0, 0, 63};

    EXPECT_TRUE(is_valid(pc_parameters, maximum_component_value, 0));
}

TEST(jpegls_preset_coding_parameters_test, is_default_nothing_set)
{
    const auto default_parameters{compute_default(255, 0)};

    constexpr jpegls_pc_parameters pc_parameters{};

    EXPECT_TRUE(is_default(pc_parameters, default_parameters));
}

TEST(jpegls_preset_coding_parameters_test, is_default_same_as_default)
{
    const auto default_parameters{compute_default(255, 0)};

    const jpegls_pc_parameters pc_parameters{compute_default(255, 0)};

    EXPECT_TRUE(is_default(pc_parameters, default_parameters));
}

TEST(jpegls_preset_coding_parameters_test, is_default_same_as_default_except_threshold2)
{
    const auto default_parameters{compute_default(255, 0)};

    jpegls_pc_parameters pc_parameters{compute_default(255, 0)};
    ++pc_parameters.threshold2;

    EXPECT_FALSE(is_default(pc_parameters, default_parameters));
}

TEST(jpegls_preset_coding_parameters_test, is_default_same_as_default_except_threshold3)
{
    const auto default_parameters{compute_default(255, 0)};

    jpegls_pc_parameters pc_parameters{compute_default(255, 0)};
    ++pc_parameters.threshold3;

    EXPECT_FALSE(is_default(pc_parameters, default_parameters));
}

TEST(jpegls_preset_coding_parameters_test, is_default_same_as_default_except_reset)
{
    const auto default_parameters{compute_default(255, 0)};

    jpegls_pc_parameters pc_parameters{compute_default(255, 0)};
    ++pc_parameters.reset_value;

    EXPECT_FALSE(is_default(pc_parameters, default_parameters));
}

} // namespace charls::test
