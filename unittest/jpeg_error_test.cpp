// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include "pch.h"

#include <charls/jpegls_error.h>

#include <array>
#include <vector>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using Microsoft::VisualStudio::CppUnitTestFramework::TestClass;
using namespace charls;
using std::array;
using std::vector;

namespace CharLSUnitTest {

// clang-format off

TEST_CLASS(jpegls_error_test)
{
public:
    TEST_METHOD(get_error_message_success)
    {
        auto result = charls_get_error_message(0);
        Assert::IsNotNull(result);
        Assert::IsTrue(strlen(result) > 0);
    }

    TEST_METHOD(get_error_message_unknown)
    {
        constexpr int32_t unknown_error_code{3000};
        auto result = charls_get_error_message(unknown_error_code);
        Assert::IsNotNull(result);
        Assert::IsTrue(strlen(result) > 0);
    }

    TEST_METHOD(jpegls_category_name_is_not_an_empty_string)
    {
        Assert::IsTrue(strlen(jpegls_category().name()) > 0);
    }
};

} // namespace CharLSUnitTest
