//
// (C) CharLS Team 2017, all rights reserved. See the accompanying "License.txt" for licensed use.
//

#pragma once

#pragma warning(push)
#pragma warning(disable : 26432)
#pragma warning(disable : 26433)
#pragma warning(disable : 26439)
#pragma warning(disable : 26440)
#pragma warning(disable : 26443)
#pragma warning(disable : 26461)
#pragma warning(disable : 26466)
#pragma warning(disable : 26495)
#pragma warning(disable : 26496)
#include <CppUnitTest.h>
#pragma warning(pop)

#include <memory>
#include <cstdint>
#include <vector>

#ifdef _MSC_VER
#define WARNING_SUPPRESS(x) __pragma(warning(push)) __pragma(warning(disable : x))  // NOLINT(misc-macro-parentheses)
#define WARNING_UNSUPPRESS() __pragma(warning(pop))
#else
#define WARNING_SUPPRESS(x)
#define WARNING_UNSUPPRESS()
#endif
