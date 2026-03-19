// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <sstream>
#include <vector>

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
#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x)
#endif
