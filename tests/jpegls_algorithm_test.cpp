// SPDX-FileCopyrightText: © 2021 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>

#include "../src/conditional_static_cast.hpp"
#include "../src/jpegls_algorithm.hpp"

#include <limits>

using std::numeric_limits;

namespace charls::test {

namespace {

void call_and_compare_log2_ceil(const int32_t value)
{
    const int32_t expected{static_cast<int32_t>(ceil(std::log2(value)))};
    EXPECT_EQ(expected, log2_ceiling(value));
}

/// <summary>
/// This is the original algorithm of ISO/IEC 14495-1, A.5.2, Code Segment A.11 (second else branch)
/// It will map signed values to unsigned values.
/// </summary>
[[nodiscard]]
int32_t map_error_value_original(const int32_t error_value) noexcept
{
    if (error_value >= 0)
        return 2 * error_value;

    return -2 * error_value - 1;
}

/// <remarks>
/// This version will be auto optimized by GCC(trunk, not 10.2) and clang(11.0). MSVC will create branches.
/// </remarks>
[[nodiscard]]
int32_t map_error_value_alternative1(const int32_t error_value) noexcept
{
    const int32_t mapped_value{error_value * 2};

    if (error_value >= 0)
        return mapped_value;

    return (-1 * mapped_value) - 1;
}

/// <summary>
/// This is the original inverse algorithm of ISO/IEC 14495-1, A.5.2, Code Segment A.11 (second else branch)
/// It will map unsigned values back to unsigned values.
/// </summary>
[[nodiscard]]
int32_t unmap_error_value_original(const int32_t mapped_error_value) noexcept
{
    if (mapped_error_value % 2 == 0)
        return mapped_error_value / 2;

    return (mapped_error_value / -2) - 1;
}

/// <remarks>
/// This version will be auto optimized by GCC(trunk, not 10.2) using a cmove,
/// and clang(11.0) 8 instructions, MSVC will create branches.
/// Optimized version uses 6 to 5 instructions.
/// </remarks>
[[nodiscard]]
int32_t unmap_error_value_alternative1(const int32_t mapped_error_value) noexcept
{
    const int32_t error_value{mapped_error_value / 2};

    if (mapped_error_value % 2 == 0)
        return error_value;

    return (-1 * error_value) - 1;
}

void map_error_value_algorithm(const int32_t error_value)
{
    const int32_t actual{map_error_value(error_value)};
    const int32_t expected1{map_error_value_original(error_value)};
    const int32_t expected2{map_error_value_alternative1(error_value)};

    EXPECT_TRUE(actual >= 0);
    EXPECT_EQ(expected1, actual);
    EXPECT_EQ(expected2, actual);
}

void unmap_error_value_algorithm(const int32_t mapped_error_value)
{
    const int32_t actual{unmap_error_value(mapped_error_value)};
    const int32_t expected1{unmap_error_value_original(mapped_error_value)};
    const int32_t expected2{unmap_error_value_alternative1(mapped_error_value)};

    EXPECT_EQ(expected1, actual);
    EXPECT_EQ(expected2, actual);
}

void map_unmap_error_value_algorithm(const int32_t error_value)
{
    const int32_t mapped_error_value{map_error_value(error_value)};
    const int32_t actual{unmap_error_value(mapped_error_value)};

    EXPECT_EQ(error_value, actual);
}

} // namespace

TEST(jpegls_algorithm_test, log2_ceil)
{
    call_and_compare_log2_ceil(1);
    call_and_compare_log2_ceil(2);
    call_and_compare_log2_ceil(31);
    call_and_compare_log2_ceil(32);
    call_and_compare_log2_ceil(33);
    call_and_compare_log2_ceil(numeric_limits<uint16_t>::max());
    call_and_compare_log2_ceil(numeric_limits<uint16_t>::max() + 1);
    call_and_compare_log2_ceil(conditional_static_cast<int32_t>(numeric_limits<uint32_t>::max() >> 2));
}

TEST(jpegls_algorithm_test, test_initialization_value_for_a)
{
    constexpr int32_t min_value{initialization_value_for_a(4)};
    constexpr int32_t max_value{initialization_value_for_a(std::numeric_limits<uint16_t>::max() + 1)};

    EXPECT_EQ(2, min_value);
    EXPECT_EQ(1024, max_value);
}

TEST(jpegls_algorithm_test, map_error_value_algorithm)
{
    map_error_value_algorithm(0);
    map_error_value_algorithm(1);
    map_error_value_algorithm(-1);
    map_error_value_algorithm(numeric_limits<int16_t>::max());
    map_error_value_algorithm(numeric_limits<int16_t>::min());
    map_error_value_algorithm(numeric_limits<int32_t>::max() / 2);
    map_error_value_algorithm(numeric_limits<int32_t>::min() / 2);
}

TEST(jpegls_algorithm_test, unmap_error_value_algorithm)
{
    unmap_error_value_algorithm(0);
    unmap_error_value_algorithm(1);
    unmap_error_value_algorithm(2);
    unmap_error_value_algorithm(numeric_limits<int16_t>::max());
    unmap_error_value_algorithm(numeric_limits<int32_t>::max() - 2);
    unmap_error_value_algorithm(numeric_limits<int32_t>::max() - 1);
    unmap_error_value_algorithm(numeric_limits<int32_t>::max());
}

TEST(jpegls_algorithm_test, map_unmap_error_value_algorithm)
{
    map_unmap_error_value_algorithm(0);
    map_unmap_error_value_algorithm(1);
    map_unmap_error_value_algorithm(-1);
    map_unmap_error_value_algorithm(numeric_limits<int16_t>::max());
    map_unmap_error_value_algorithm(numeric_limits<int16_t>::min());
    map_unmap_error_value_algorithm(numeric_limits<int32_t>::max() / 2);
    map_unmap_error_value_algorithm(numeric_limits<int32_t>::min() / 2);
}

} // namespace charls::test
