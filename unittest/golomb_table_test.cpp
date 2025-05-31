// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "util.hpp"

#include "../src/golomb_lut.hpp"


using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace charls::test {

TEST_CLASS(golomb_table_test)
{
public:
    TEST_METHOD(golomb_table_create) // NOLINT
    {
        const golomb_code_match_table golomb_table(0);

        Assert::AreEqual(0, golomb_table.get(0).bit_count);
        Assert::AreEqual(0, golomb_table.get(0).error_value);
        Assert::AreEqual(1, golomb_table.get(255).bit_count);
        Assert::AreEqual(0, golomb_table.get(255).error_value);
    }
};

// The Windows x64 ABI has strict requirements when it is allowed to return a struct in a register.
static_assert(std::is_standard_layout_v<golomb_code_match>);
static_assert(std::is_trivial_v<golomb_code_match>);


} // namespace charls::test
