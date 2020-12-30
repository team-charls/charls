// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include "../src/lookup_table.h"


using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace charls { namespace test {

TEST_CLASS(golomb_table_test)
{
public:
    TEST_METHOD(golomb_table_create) // NOLINT
    {
        const golomb_code_table golomb_table;

        for (uint32_t i{}; i < 256U; i++)
        {
            Assert::AreEqual(0U, golomb_table.get(i).length());
            Assert::AreEqual(0, golomb_table.get(i).value());
        }
    }
};

}} // namespace charls::test
