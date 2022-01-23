// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "../src/util.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;
using std::numeric_limits;

namespace charls { namespace test {

namespace {

uint32_t log2_floor(const uint32_t n) noexcept
{
    ASSERT(n != 0 && "log2 is not defined for 0");
    return 31 - countl_zero(n);
}

uint32_t max_value_to_bits_per_sample(const uint32_t max_value) noexcept
{
    ASSERT(max_value > 0);
    return log2_floor(max_value) + 1;
}

void call_and_compare_log2_ceil(const int32_t arg)
{
    const int32_t expected{static_cast<int32_t>(ceil(std::log2(arg)))};
    Assert::AreEqual(expected, log2_ceil(arg));
}

void call_and_compare_log2_floor(const uint32_t arg)
{
    MSVC_WARNING_SUPPRESS_NEXT_LINE(26467) // cast from double to uint32 is safe. values always positive.
    const uint32_t expected{static_cast<uint32_t>(floor(std::log2(arg)))};
    Assert::AreEqual(expected, log2_floor(arg));
}

}

TEST_CLASS(util_test)
{
public:
    TEST_METHOD(log2_ceil) // NOLINT
    {
        call_and_compare_log2_ceil(1);
        call_and_compare_log2_ceil(2);
        call_and_compare_log2_ceil(32);
        call_and_compare_log2_ceil(33);
        call_and_compare_log2_ceil(33);
        call_and_compare_log2_ceil(numeric_limits<uint16_t>::max());
        call_and_compare_log2_ceil(numeric_limits<uint16_t>::max() + 1);
        call_and_compare_log2_ceil(numeric_limits<uint32_t>::max() >> 2);
    }

    TEST_METHOD(log2_floor) // NOLINT
    {
        call_and_compare_log2_floor(1);
        call_and_compare_log2_floor(2);
        call_and_compare_log2_floor(32);
        call_and_compare_log2_floor(33);
        call_and_compare_log2_floor(33);
        call_and_compare_log2_floor(numeric_limits<uint16_t>::max());
        call_and_compare_log2_floor(numeric_limits<uint16_t>::max() + 1);
        call_and_compare_log2_floor(numeric_limits<uint32_t>::max() >> 2);
    }

    TEST_METHOD(max_value_to_bits_per_sample_test) // NOLINT
    {
        Assert::AreEqual(1U, max_value_to_bits_per_sample(1));
        Assert::AreEqual(2U, max_value_to_bits_per_sample(2));
        Assert::AreEqual(5U, max_value_to_bits_per_sample(31));
        Assert::AreEqual(6U, max_value_to_bits_per_sample(32));
        Assert::AreEqual(6U, max_value_to_bits_per_sample(33));
        Assert::AreEqual(8U, max_value_to_bits_per_sample(255));
        Assert::AreEqual(10U, max_value_to_bits_per_sample(1023));
        Assert::AreEqual(16U, max_value_to_bits_per_sample(numeric_limits<uint16_t>::max()));
    }
};

}} // namespace charls::test
