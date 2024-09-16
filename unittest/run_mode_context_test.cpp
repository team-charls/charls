// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "../src/run_mode_context.hpp"


using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace charls::test {

TEST_CLASS(run_mode_context_test)
{
public:
    TEST_METHOD(update_variable) // NOLINT
    {
        run_mode_context context{0, 4};

        context.update_variables(3, 27, 0);

        Assert::AreEqual(3, context.compute_golomb_coding_parameter());
        Assert::AreEqual(3, context.compute_golomb_coding_parameter_checked());
    }
};

} // namespace charls::test
