// SPDX-FileCopyrightText: © 2019 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include <charls/version.hpp>

#include "support.hpp"

#include <cstring>
#include <sstream>

using std::ostringstream;

namespace charls::test {

TEST(version_test, charls_get_version_string_test)
{
    const char* version{charls_get_version_string()};

    ostringstream expected_stream;
    expected_stream << version_major << "." << version_minor << "." << version_patch;
    const auto expected{expected_stream.str()};

    EXPECT_TRUE(strncmp(expected.c_str(), version, expected.length()) == 0);
    if (expected.length() != strlen(version))
    {
        EXPECT_EQ('-', version[expected.length()]);
    }
}

TEST(version_test, version_string_test)
{
    const char* version{version_string()};

    ostringstream expected_stream;
    expected_stream << CHARLS_VERSION_MAJOR << "." << CHARLS_VERSION_MINOR << "." << CHARLS_VERSION_PATCH;
    const auto expected{expected_stream.str()};

    EXPECT_TRUE(strncmp(expected.c_str(), version, expected.length()) == 0);
    if (expected.length() != strlen(version))
    {
        EXPECT_EQ('-', version[expected.length()]);
    }
}

TEST(version_test, charls_get_version_number_all)
{
    int32_t major;
    int32_t minor;
    int32_t patch;

    charls_get_version_number(&major, &minor, &patch);

    // Explicitly use the C macros.
    EXPECT_EQ(CHARLS_VERSION_MAJOR, major);
    EXPECT_EQ(CHARLS_VERSION_MINOR, minor);
    EXPECT_EQ(CHARLS_VERSION_PATCH, patch);
}

TEST(version_test, charls_get_version_number_none)
{
    charls_get_version_number(nullptr, nullptr, nullptr);

    // No explicit test possible, code should not throw and remain stable.
    EXPECT_TRUE(true);
}

TEST(version_test, version_number_test)
{
    const auto [major, minor, patch] = version_number();

    EXPECT_EQ(version_major, major);
    EXPECT_EQ(version_minor, minor);
    EXPECT_EQ(version_patch, patch);
}

} // namespace charls::test
