// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "../src/regular_mode_context.hpp"


using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace charls::test {

TEST_CLASS(regular_mode_context_test)
{
public:
    TEST_METHOD(compute_golomb_coding_parameter)
    {
        regular_mode_context context;

        Assert::AreEqual(0, context.compute_golomb_coding_parameter());
        Assert::AreEqual(0, context.compute_golomb_coding_parameter_for_encoder());

        context.update_variables_and_bias(7, 0, 64);
        Assert::AreEqual(2, context.compute_golomb_coding_parameter());
        Assert::AreEqual(2, context.compute_golomb_coding_parameter_for_encoder());
    }
};

} // namespace charls::test
