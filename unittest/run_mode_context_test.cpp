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
        run_mode_context context;

        context.update_variables(3, 27, 0);

        Assert::AreEqual(3, context.get_golomb_code());
    }
};

} // namespace charls::test
