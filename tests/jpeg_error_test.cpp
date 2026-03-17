// SPDX-FileCopyrightText: © 2019 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>

#include <charls/jpegls_error.hpp>

#include <cstring>
#include <string>
#include <type_traits>

namespace charls::test {

TEST(jpegls_error_test, get_error_message_success)
{
    const auto* const result{charls_get_error_message(jpegls_errc::success)};
    EXPECT_NE(nullptr, result);
    EXPECT_GT(strlen(result), size_t{0});
}

TEST(jpegls_error_test, get_error_message_unknown)
{
    constexpr jpegls_errc unknown_error_code{static_cast<jpegls_errc>(3000)};
    const auto* const result{charls_get_error_message(unknown_error_code)};
    EXPECT_NE(nullptr, result);
    EXPECT_GT(strlen(result), size_t{0});
}

TEST(jpegls_error_test, get_error_message_not_enough_memory)
{
    const auto* const result{charls_get_error_message(jpegls_errc::not_enough_memory)};
    EXPECT_NE(nullptr, result);
    EXPECT_GT(strlen(result), size_t{0});
}

TEST(jpegls_error_test, jpegls_category_name_is_not_an_empty_string)
{
    EXPECT_GT(strlen(jpegls_category().name()), size_t{0});
}

TEST(jpegls_error_test, is_error_code_enum)
{
    constexpr std::is_error_code_enum<jpegls_errc> test;
    EXPECT_TRUE(test);
}

TEST(jpegls_error_test, jpegls_category_call_message)
{
    const std::error_category& category{jpegls_category()};
    const std::string message{category.message(0)};
    EXPECT_FALSE(message.empty());
}

} // namespace charls::test
