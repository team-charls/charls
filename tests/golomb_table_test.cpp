// SPDX-FileCopyrightText: © 2019 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>
#include <type_traits>

#include "../src/golomb_lut.hpp"

namespace charls::test {

// The Windows x64 ABI has strict requirements when it is allowed to return a struct in a register.
static_assert(std::is_standard_layout_v<golomb_code_match>);
static_assert(std::is_trivial_v<golomb_code_match>);

TEST(golomb_table_test, golomb_table_create)
{
    const golomb_code_match_table golomb_table(0);

    EXPECT_EQ(0, golomb_table.get(0).bit_count);
    EXPECT_EQ(0, golomb_table.get(0).error_value);
    EXPECT_EQ(1, golomb_table.get(255).bit_count);
    EXPECT_EQ(0, golomb_table.get(255).error_value);
}

} // namespace charls::test
