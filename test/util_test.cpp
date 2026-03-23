// SPDX-FileCopyrightText: © 2022 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "../src/assert.hpp"
#include "../src/util.hpp"

#include "support.hpp"

#include <cmath>

using std::numeric_limits;

namespace charls::test {

namespace {

[[nodiscard]]
uint32_t log2_floor(const uint32_t n) noexcept
{
    ASSERT(n != 0 && "log2 is not defined for 0");
    return 31U - countl_zero(n);
}

[[nodiscard]]
uint32_t max_value_to_bits_per_sample(const uint32_t max_value) noexcept
{
    ASSERT(max_value > 0);
    return log2_floor(max_value) + 1;
}

void call_and_compare_log2_floor(const uint32_t arg)
{
    MSVC_WARNING_SUPPRESS_NEXT_LINE(26467) // cast from double to uint32 is safe. values always positive.
    const uint32_t expected{static_cast<uint32_t>(std::floor(std::log2(arg)))};
    EXPECT_EQ(expected, log2_floor(arg));
}

} // namespace


TEST(util_test, log2_floor)
{
    call_and_compare_log2_floor(1);
    call_and_compare_log2_floor(2);
    call_and_compare_log2_floor(31);
    call_and_compare_log2_floor(32);
    call_and_compare_log2_floor(33);
    call_and_compare_log2_floor(numeric_limits<uint16_t>::max());
    call_and_compare_log2_floor(numeric_limits<uint16_t>::max() + 1U);
    call_and_compare_log2_floor(numeric_limits<uint32_t>::max() >> 2);
}

TEST(util_test, max_value_to_bits_per_sample_test)
{
    EXPECT_EQ(1U, max_value_to_bits_per_sample(1));
    EXPECT_EQ(2U, max_value_to_bits_per_sample(2));
    EXPECT_EQ(5U, max_value_to_bits_per_sample(31));
    EXPECT_EQ(6U, max_value_to_bits_per_sample(32));
    EXPECT_EQ(6U, max_value_to_bits_per_sample(33));
    EXPECT_EQ(8U, max_value_to_bits_per_sample(255));
    EXPECT_EQ(10U, max_value_to_bits_per_sample(1023));
    EXPECT_EQ(16U, max_value_to_bits_per_sample(numeric_limits<uint16_t>::max()));
}

TEST(util_test, to_jpegls_errc_test)
{
    try
    {
        throw jpegls_error(jpegls_errc::invalid_argument);
    }
    catch (...)
    {
        EXPECT_EQ(jpegls_errc::invalid_argument, to_jpegls_errc());
    }

    try
    {
        throw std::bad_alloc();
    }
    catch (...)
    {
        EXPECT_EQ(jpegls_errc::not_enough_memory, to_jpegls_errc());
    }
}

} // namespace charls::test
