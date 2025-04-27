// SPDX-FileCopyrightText: © 2025 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cassert>

// Use an uppercase alias for assert to make it clear that ASSERT is a pre-processor macro.
#ifdef _MSC_VER
// C26493 = Don't use C-style casts
#define ASSERT(expression) \
    __pragma(warning(push)) __pragma(warning(disable : 26493)) assert(expression) __pragma(warning(pop))
#else
#define ASSERT(expression) assert(expression)
#endif
