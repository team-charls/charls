// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include "../src/lookup_table.h"


using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

// clang-format off

namespace charls {
namespace test {

TEST_CLASS(ctable_test)
{
public:
    TEST_METHOD(CTable_create) // NOLINT
    {
        const golomb_code_table golomb_table;

        for (uint32_t i = 0U; i < 256U; i++)
        {
            Assert::AreEqual(0U, golomb_table.Get(i).length());
            Assert::AreEqual(0, golomb_table.Get(i).value());
        }
    }
};

} // namespace test
} // namespace charls
