// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "../src/regular_mode_context.hpp"

namespace charls::test {

TEST(regular_mode_context_test, compute_golomb_coding_parameter)
{
    regular_mode_context context;

    EXPECT_EQ(0, context.compute_golomb_coding_parameter());
    EXPECT_EQ(0, context.compute_golomb_coding_parameter_for_encoder());

    context.update_variables_and_bias(7, 0, 64);
    EXPECT_EQ(2, context.compute_golomb_coding_parameter());
    EXPECT_EQ(2, context.compute_golomb_coding_parameter_for_encoder());
}

} // namespace charls::test
