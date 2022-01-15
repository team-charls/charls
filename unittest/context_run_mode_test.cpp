// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "../src/context_run_mode.h"


using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace charls { namespace test {

TEST_CLASS(context_run_mode_test)
{
public:
    TEST_METHOD(update_variable) // NOLINT
    {
        context_run_mode context;

        context.update_variables(3, 27, 0);

        Assert::AreEqual(3, context.get_golomb_code());
    }
};

}} // namespace charls::test
