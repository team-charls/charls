// SPDX-FileCopyrightText: © 2022 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>

#include "../src/run_mode_context.hpp"

namespace charls::test {

TEST(run_mode_context_test, update_variable)
{
    run_mode_context context{0, 4};

    context.update_variables(3, 27, 0);

    EXPECT_EQ(3, context.compute_golomb_coding_parameter());
    EXPECT_EQ(3, context.compute_golomb_coding_parameter_checked());
}

} // namespace charls::test
