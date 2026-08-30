// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

// IWYU pragma: begin_exports
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <string>
#include <limits>
#include <memory>
#include <sstream>
#include <vector>
#include <filesystem>
// IWYU pragma: end_exports

#ifdef _MSC_VER
#define MSVC_WARNING_SUPPRESS(x) \
    __pragma(warning(push)) __pragma(warning(disable : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses)
#define MSVC_WARNING_UNSUPPRESS() __pragma(warning(pop))

#pragma warning(disable : 6326)  // Potential comparison of a constant with another constant: triggered by EXPECT_EQ.
#pragma warning(disable : 26818) // Switch statement does not have a default case: triggered by EXPECT_EQ.
#pragma warning(disable : 26409) // Avoid calling new and delete explicitly, use std::make_unique<T>: triggered by TEST.
#pragma warning(disable : 26440) // Function can be declared noexcept: triggered by TEST.
#pragma warning(disable : 26455) // Default constructor should not throw.Declare it 'noexcept'(f.6): triggered by TEST.

#else
#define MSVC_WARNING_SUPPRESS(x)
#define MSVC_WARNING_UNSUPPRESS()
#endif
