// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/version.h>

#include <sstream>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::ostringstream;

namespace charls { namespace test {

TEST_CLASS(version_test)
{
public:
    TEST_METHOD(charls_get_version_string_test) // NOLINT
    {
        const char* version{charls_get_version_string()};

        ostringstream expected_stream;
        expected_stream << CHARLS_VERSION_MAJOR << "." << CHARLS_VERSION_MINOR << "." << CHARLS_VERSION_PATCH;
        const auto expected{expected_stream.str()};

        Assert::IsTrue(strncmp(expected.c_str(), version, expected.length()) == 0);
        if (expected.length() != strlen(version))
        {
            Assert::AreEqual('-', version[expected.length()]);
        }
    }

    TEST_METHOD(charls_get_version_number_all) // NOLINT
    {
        int32_t major;
        int32_t minor;
        int32_t patch;

        charls_get_version_number(&major, &minor, &patch);

        Assert::AreEqual(CHARLS_VERSION_MAJOR, major);
        Assert::AreEqual(CHARLS_VERSION_MINOR, minor);
        Assert::AreEqual(CHARLS_VERSION_PATCH, patch);
    }

    TEST_METHOD(charls_get_version_number_none) // NOLINT
    {
        charls_get_version_number(nullptr, nullptr, nullptr);

        // No explicit test possible, code should not throw and remain stable.
        Assert::IsTrue(true);
    }
};

}} // namespace charls::test
