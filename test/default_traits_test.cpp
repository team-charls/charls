// SPDX-FileCopyrightText: © 2016 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "../src/default_traits.hpp"

namespace charls::test {

TEST(default_traits_test, create)
{
    const default_traits<uint8_t, uint8_t> traits((1 << 8) - 1, 0);

    EXPECT_EQ(255, traits.maximum_sample_value);
    EXPECT_EQ(256, traits.range);
    EXPECT_EQ(0, traits.near_lossless);
    EXPECT_EQ(8, traits.quantized_bits_per_sample);
    EXPECT_EQ(8, traits.bits_per_sample);
    EXPECT_EQ(32, traits.limit);
}

TEST(default_traits_test, modulo_range)
{
    const default_traits<uint8_t, uint8_t> traits(24, 0);

    for (int i{-25}; i != 26; ++i)
    {
        const auto error_value{traits.modulo_range(i)};
        constexpr int range{24 + 1};
        EXPECT_TRUE(-range / 2 <= error_value && error_value <= ((range + 1) / 2) - 1);
    }
}

} // namespace charls::test
