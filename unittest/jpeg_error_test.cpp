// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include <charls/jpegls_error.h>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using Microsoft::VisualStudio::CppUnitTestFramework::TestClass;

namespace charls { namespace test {

TEST_CLASS(jpegls_error_test)
{
public:
    TEST_METHOD(get_error_message_success) // NOLINT
    {
        const auto* const result{charls_get_error_message(jpegls_errc::success)};
        Assert::IsNotNull(result);
        Assert::IsTrue(strlen(result) > 0);
    }

    TEST_METHOD(get_error_message_unknown) // NOLINT
    {
        constexpr jpegls_errc unknown_error_code{static_cast<jpegls_errc>(3000)};
        const auto* const result{charls_get_error_message(unknown_error_code)};
        Assert::IsNotNull(result);
        Assert::IsTrue(strlen(result) > 0);
    }

    TEST_METHOD(jpegls_category_name_is_not_an_empty_string) // NOLINT
    {
        Assert::IsTrue(strlen(jpegls_category().name()) > 0);
    }

    TEST_METHOD(jpegls_category_call_message) // NOLINT
    {
        const std::error_category& category{jpegls_category()};

        std::string message{category.message(0)};
        Assert::IsTrue(message.size() > 0);
    }
};

}} // namespace charls::test
