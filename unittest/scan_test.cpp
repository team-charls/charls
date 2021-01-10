// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include "../src/scan.h"


using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::vector;

namespace {

/// <summary>
/// This is the original algorithm of ISO/IEC 14495-1, A.5.2, Code Segment A.11 (second else branch)
/// It will map signed values to unsiged values.
/// </summary>
int32_t map_error_value_original(const int32_t error_value) noexcept
{
    if (error_value >= 0)
        return 2 * error_value;

    return -2 * error_value - 1;
}

/// <remarks>
/// This version will be auto optimized by GCC(trunk, not 10.2) and clang(11.0). MSVC will create branches.
/// </remarks>
int32_t map_error_value_alternative1(const int32_t error_value) noexcept
{
    const int32_t mapped_value = error_value * 2;

    if (error_value >= 0)
        return mapped_value;

    return (-1 * mapped_value) - 1;
}


/// <summary>
/// This is the original inverse algorithm of ISO/IEC 14495-1, A.5.2, Code Segment A.11 (second else branch)
/// It will map unsigned values back to unsigned values.
/// </summary>
int32_t unmap_error_value_original(const int32_t mapped_error_value) noexcept
{
    if (mapped_error_value % 2 == 0)
        return mapped_error_value / 2;

    return (mapped_error_value / -2) - 1;
}

/// <remarks>
/// This version will be auto optimized by GCC(trunk, not 10.2) using a cmove,
/// and clang(11.0) 8 instructions,. MSVC will create branches.
/// Optimized version uses 6 to 5 instructions.
/// </remarks>
int32_t unmap_error_value_alternative1(const int32_t mapped_error_value) noexcept
{
    const int32_t error_value = mapped_error_value / 2;

    if (mapped_error_value % 2 == 0)
        return error_value;

    return (-1 * error_value) - 1;
}

}

namespace charls { namespace test {

TEST_CLASS(scan_test)
{
public:
    TEST_METHOD(map_error_value_algorithm) // NOLINT
    {
        map_error_value_algorithm(0);
        map_error_value_algorithm(1);
        map_error_value_algorithm(-1);
        map_error_value_algorithm(INT16_MAX);
        map_error_value_algorithm(INT16_MIN);
        map_error_value_algorithm(INT32_MAX / 2);
        map_error_value_algorithm(INT32_MIN / 2);
    }

    TEST_METHOD(unmap_error_value_algorithm) // NOLINT
    {
        unmap_error_value_algorithm(0);
        unmap_error_value_algorithm(1);
        unmap_error_value_algorithm(2);
        unmap_error_value_algorithm(INT16_MAX);
        unmap_error_value_algorithm(INT32_MAX - 2);
        unmap_error_value_algorithm(INT32_MAX - 1);
        unmap_error_value_algorithm(INT32_MAX);
    }

    TEST_METHOD(map_unmap_error_value_algorithm) // NOLINT
    {
        map_unmap_error_value_algorithm(0);
        map_unmap_error_value_algorithm(1);
        map_unmap_error_value_algorithm(-1);
        map_unmap_error_value_algorithm(INT16_MAX);
        map_unmap_error_value_algorithm(INT16_MIN);
        map_unmap_error_value_algorithm(INT32_MAX / 2);
        map_unmap_error_value_algorithm(INT32_MIN / 2);
    }

private:
    static void map_error_value_algorithm(const int32_t error_value)
    {
        const int32_t actual = map_error_value(error_value);
        const int32_t expected1 = map_error_value_original(error_value);
        const int32_t expected2 = map_error_value_alternative1(error_value);

        Assert::IsTrue(actual >= 0);
        Assert::AreEqual(expected1, actual);
        Assert::AreEqual(expected2, actual);
    }

    static void unmap_error_value_algorithm(const int32_t mapped_error_value)
    {
        const int32_t actual = unmap_error_value(mapped_error_value);
        const int32_t expected1 = unmap_error_value_original(mapped_error_value);
        const int32_t expected2 = unmap_error_value_alternative1(mapped_error_value);

        Assert::AreEqual(expected1, actual);
        Assert::AreEqual(expected2, actual);
    }

    static void map_unmap_error_value_algorithm(const int32_t error_value)
    {
        const int32_t mapped_error_value = map_error_value(error_value);
        const int32_t actual = unmap_error_value(mapped_error_value);

        Assert::AreEqual(error_value, actual);
    }
};

}} // namespace charls::test
