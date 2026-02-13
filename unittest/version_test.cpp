// SPDX-FileCopyrightText: © 2019 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "util.hpp"

#include <charls/version.hpp>

#include <sstream>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::ostringstream;

namespace charls::test {

TEST_CLASS(version_test)
{
public:
    TEST_METHOD(charls_get_version_string_test)
    {
        const char* version{charls_get_version_string()};

        ostringstream expected_stream;
        expected_stream << version_major << "." << version_minor << "." << version_patch;
        const auto expected{expected_stream.str()};

        Assert::IsTrue(strncmp(expected.c_str(), version, expected.length()) == 0);
        if (expected.length() != strlen(version))
        {
            Assert::AreEqual('-', version[expected.length()]);
        }
    }

    TEST_METHOD(version_string_test)
    {
        const char* version{version_string()};

        ostringstream expected_stream;
        expected_stream << CHARLS_VERSION_MAJOR << "." << CHARLS_VERSION_MINOR << "." << CHARLS_VERSION_PATCH;
        const auto expected{expected_stream.str()};

        Assert::IsTrue(strncmp(expected.c_str(), version, expected.length()) == 0);
        if (expected.length() != strlen(version))
        {
            Assert::AreEqual('-', version[expected.length()]);
        }
    }

    TEST_METHOD(charls_get_version_number_all)
    {
        int32_t major;
        int32_t minor;
        int32_t patch;

        charls_get_version_number(&major, &minor, &patch);

        // Use explicit the C macros.
        Assert::AreEqual(CHARLS_VERSION_MAJOR, major);
        Assert::AreEqual(CHARLS_VERSION_MINOR, minor);
        Assert::AreEqual(CHARLS_VERSION_PATCH, patch);
    }

    TEST_METHOD(charls_get_version_number_none)
    {
        charls_get_version_number(nullptr, nullptr, nullptr);

        // No explicit test possible, code should not throw and remain stable.
        Assert::IsTrue(true);
    }

    TEST_METHOD(version_number_test)
    {
        const auto [major, minor, patch] = version_number();

        Assert::AreEqual(version_major, major);
        Assert::AreEqual(version_minor, minor);
        Assert::AreEqual(version_patch, patch);
    }
};

} // namespace charls::test
