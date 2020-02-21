// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/charls.h>

#include "../test/portable_anymap_file.h"

#include <CppUnitTest.h>

#include <vector>

namespace charls {
namespace test {

std::vector<uint8_t> read_file(const char* filename);

charls_test::portable_anymap_file read_anymap_reference_file(const char* filename, charls::interleave_mode interleave_mode, const charls::frame_info& frame_info);
charls_test::portable_anymap_file read_anymap_reference_file(const char* filename, charls::interleave_mode interleave_mode);
std::vector<uint8_t> create_test_spiff_header(uint8_t high_version = 2, uint8_t low_version = 0, bool end_of_directory = true);
std::vector<uint8_t> create_noise_image_16bit(size_t pixel_count, int bit_count, uint32_t seed);
void test_round_trip_legacy(const std::vector<uint8_t>& source, const JlsParameters& params);

}
} // namespace charls::test

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

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

}
}
} // namespace Microsoft::VisualStudio::CppUnitTestFramework


namespace charls {
namespace test {

template<typename Functor>
void assert_expect_exception(jpegls_errc error_value, Functor functor)
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

}
} // namespace charls::test
