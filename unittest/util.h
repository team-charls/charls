// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/charls.h>

#include "../test/portable_anymap_file.h"

#include <CppUnitTest.h>

#include <vector>


// clang-format off
#ifdef __clang__
#define DISABLE_DEPRECATED_WARNING \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#elif defined(__GNUC__) || defined(__GNUG__)
#define DISABLE_DEPRECATED_WARNING \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#elif defined(_MSC_VER)
#define DISABLE_DEPRECATED_WARNING \
    __pragma(warning(push)) \
    __pragma(warning(disable : 4996)) // was declared deprecated
#else
#define DISABLE_DEPRECATED_WARNING
#endif
// clang-format on

#ifdef __clang__
#define RESTORE_DEPRECATED_WARNING _Pragma("clang diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
#define RESTORE_DEPRECATED_WARNING _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
#define RESTORE_DEPRECATED_WARNING __pragma(warning(pop))
#else
#define RESTORE_DEPRECATED_WARNING
#endif


namespace charls { namespace test {

std::vector<uint8_t> read_file(const char* filename);

charls_test::portable_anymap_file read_anymap_reference_file(const char* filename, interleave_mode interleave_mode,
                                                             const frame_info& frame_info);
charls_test::portable_anymap_file read_anymap_reference_file(const char* filename, interleave_mode interleave_mode);
std::vector<uint8_t> create_test_spiff_header(uint8_t high_version = 2, uint8_t low_version = 0,
                                              bool end_of_directory = true);
std::vector<uint8_t> create_noise_image_16_bit(size_t pixel_count, int bit_count, uint32_t seed);
void test_round_trip_legacy(const std::vector<uint8_t>& source, const JlsParameters& params);

/// <summary>
/// Computes how many bytes are needed to hold the number of bits.
/// </summary>
constexpr uint32_t bit_to_byte_count(const int32_t bit_count) noexcept
{
    return static_cast<uint32_t>((bit_count + 7) / 8);
}

}} // namespace charls::test

// ReSharper disable CppInconsistentNaming
namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework {
// ReSharper restore CppInconsistentNaming

template<>
inline std::wstring ToString<charls::jpegls_errc>(const charls::jpegls_errc& q)
{
    RETURN_WIDE_STRING(static_cast<int>(q));
}

template<>
inline std::wstring ToString<charls::interleave_mode>(const charls::interleave_mode& q)
{
    RETURN_WIDE_STRING(static_cast<int>(q));
}

}}} // namespace Microsoft::VisualStudio::CppUnitTestFramework


namespace charls { namespace test {

template<typename Functor>
void assert_expect_exception(const jpegls_errc error_value, Functor functor)
{
    try
    {
        functor();
    }
    catch (const jpegls_error& error)
    {
        Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(error_value == error.code());
        Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsNotNull(error.what());
        Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(strlen(error.what()) > 0);
        return;
    }
    catch (...)
    {
        throw;
    }

    Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail();
}

}} // namespace charls::test
