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
        const CTable golomb_table;

        for (int i = 0; i < 256; i++)
        {
            Assert::AreEqual(0, golomb_table.Get(i).GetLength());
            Assert::AreEqual(0, golomb_table.Get(i).GetValue());
        }
    }
};

} // namespace test
} // namespace charls
