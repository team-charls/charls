// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#pragma warning(push)
#pragma warning(disable : 5039) // '_set_invalid_parameter_handler' : pointer or reference to potentially throwing function passed to extern C function under - EHc.Undefined behavior may occur if this function throws an exception.
#pragma warning(disable : 26432)
#pragma warning(disable : 26433)
#pragma warning(disable : 26439)
#pragma warning(disable : 26440)
#pragma warning(disable : 26443)
#pragma warning(disable : 26455) // Default constructor may not throw. Declare it 'noexcept' (f.6). [Problem in VS 2017 15.8.0]
#pragma warning(disable : 26461)
#pragma warning(disable : 26466)
#pragma warning(disable : 26477) // Use 'nullptr' rather than 0 or NULL (es.47) [Problem in VS 2017 15.8.0]
#pragma warning(disable : 26495)
#pragma warning(disable : 26496)
#include <CppUnitTest.h>
#pragma warning(pop)

#include <memory>
#include <cstdint>
#include <vector>

#ifdef _MSC_VER
#define MSVC_WARNING_SUPPRESS(x) __pragma(warning(push)) __pragma(warning(disable : x))  // NOLINT(misc-macro-parentheses)
#define MSVC_WARNING_UNSUPPRESS() __pragma(warning(pop))
#else
#define MSVC_WARNING_SUPPRESS(x)
#define MSVC_WARNING_UNSUPPRESS()
#endif
