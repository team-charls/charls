// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

// Must include charls_legacy.h first as charls.h will undefine macro's
#include <charls/charls_legacy.h>

#include <charls/charls.h>

#include <exception>
#include <vector>

struct Size final
{
    Size(const size_t width, const size_t height) noexcept :
        cx(width),
        cy(height)
    {
    }
    size_t cx;
    size_t cy;
};


void FixEndian(std::vector<uint8_t>* buffer, bool littleEndianData) noexcept;
std::vector<uint8_t> ReadFile(const char* filename, long offset = 0, size_t bytes = 0);
void TestFile(const char* filename, int offset, Size size2, int bitsPerSample, int componentCount, bool littleEndianFile = false, int loopCount = 1);
void TestRoundTrip(const char* strName, const std::vector<uint8_t>& decodedBuffer, Size size, int bitsPerSample, int componentCount, int loopCount = 1);
void TestRoundTrip(const char* strName, const std::vector<uint8_t>& originalBuffer, JlsParameters& params, int loopCount = 1);
void test_portable_anymap_file(const char* filename, int loopCount = 1);

class UnitTestException final : public std::exception
{
public:
    explicit UnitTestException() = default;
};

class Assert final
{
public:
    static void IsTrue(const bool condition)
    {
        if (!condition)
            throw UnitTestException();
    }
};

#ifdef _MSC_VER
#define MSVC_WARNING_SUPPRESS(x) __pragma(warning(push)) __pragma(warning(disable \
                                                                          : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses)
#define MSVC_WARNING_UNSUPPRESS() __pragma(warning(pop))
#else
#define MSVC_WARNING_SUPPRESS(x)
#define MSVC_WARNING_UNSUPPRESS()
#endif
