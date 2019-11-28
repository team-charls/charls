// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include "../src/lookup_table.h"


using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using namespace charls;

// clang-format off

namespace CharLSUnitTest {

TEST_CLASS(ctable_test)
{
public:
    TEST_METHOD(CTable_create)
    {
        const CTable golomb_table;

        for (int i = 0; i < 256; i++)
        {
            Assert::AreEqual(0, golomb_table.Get(i).GetLength());
            Assert::AreEqual(0, golomb_table.Get(i).GetValue());
        }
    }
};

} // namespace CharLSUnitTest
