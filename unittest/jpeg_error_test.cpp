// SPDX-FileCopyrightText: © 2019 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include <charls/jpegls_error.hpp>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using Microsoft::VisualStudio::CppUnitTestFramework::TestClass;

namespace charls::test {

TEST_CLASS(jpegls_error_test)
{
public:
    TEST_METHOD(get_error_message_success)
    {
        const auto* const result{charls_get_error_message(jpegls_errc::success)};
        Assert::IsNotNull(result);
        Assert::IsTrue(strlen(result) > 0);
    }

    TEST_METHOD(get_error_message_unknown)
    {
        constexpr jpegls_errc unknown_error_code{static_cast<jpegls_errc>(3000)};
        const auto* const result{charls_get_error_message(unknown_error_code)};
        Assert::IsNotNull(result);
        Assert::IsTrue(strlen(result) > 0);
    }

    TEST_METHOD(jpegls_category_name_is_not_an_empty_string)
    {
        Assert::IsTrue(strlen(jpegls_category().name()) > 0);
    }

    TEST_METHOD(is_error_code_enum)
    {
        constexpr std::is_error_code_enum<jpegls_errc> test;

        Assert::IsTrue(test);
    }

    TEST_METHOD(jpegls_category_call_message)
    {
        const std::error_category& category{jpegls_category()};

        const std::string message{category.message(0)};
        Assert::IsFalse(message.empty());
    }
};

} // namespace charls::test
