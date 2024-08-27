// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/public_types.h>
#include <charls/jpegls_error.hpp>

#include "../test/portable_anymap_file.hpp"

#include <CppUnitTest.h>

#include <vector>


namespace charls::test {

[[nodiscard]]
std::vector<std::byte> read_file(const char* filename);

[[nodiscard]]
charls_test::portable_anymap_file read_anymap_reference_file(const char* filename, interleave_mode interleave_mode,
                                                             const frame_info& frame_info);

[[nodiscard]]
charls_test::portable_anymap_file read_anymap_reference_file(const char* filename, interleave_mode interleave_mode);

[[nodiscard]]
std::vector<std::byte> create_test_spiff_header(uint8_t high_version = 2, uint8_t low_version = 0,
                                                bool end_of_directory = true, uint8_t component_count = 3);

[[nodiscard]]
std::vector<std::byte> create_noise_image_16_bit(size_t pixel_count, int bit_count, uint32_t seed);

[[nodiscard]]
bool verify_encoded_bytes(const std::vector<std::byte>& uncompressed_source, const std::vector<std::byte>& encoded_source);

void verify_decoded_bytes(interleave_mode interleave_mode, const frame_info& frame_info,
                          const std::vector<std::byte>& uncompressed_data, size_t destination_stride,
                          const char* reference_filename);
void test_compliance(const std::vector<std::byte>& encoded_source, const std::vector<std::byte>& uncompressed_source,
                     bool check_encode);


/// <summary>
/// Computes how many bytes are needed to hold the number of bits.
/// </summary>
[[nodiscard]]
constexpr uint32_t bit_to_byte_count(const int32_t bit_count) noexcept
{
    return static_cast<uint32_t>((bit_count + 7) / 8);
}

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

} // namespace charls::test


template<>
inline std::wstring
Microsoft::VisualStudio::CppUnitTestFramework::ToString<charls::jpegls_errc>(const charls::jpegls_errc& q)
{
    RETURN_WIDE_STRING(static_cast<int>(q));
}

template<>
inline std::wstring
Microsoft::VisualStudio::CppUnitTestFramework::ToString<charls::interleave_mode>(const charls::interleave_mode& q)
{
    RETURN_WIDE_STRING(static_cast<int>(q));
}

template<>
inline std::wstring Microsoft::VisualStudio::CppUnitTestFramework::ToString<charls::compressed_data_format>(
    const charls::compressed_data_format& q)
{
    RETURN_WIDE_STRING(static_cast<int>(q));
}

template<>
inline std::wstring Microsoft::VisualStudio::CppUnitTestFramework::ToString<std::byte>(const std::byte& q)
{
    RETURN_WIDE_STRING(static_cast<int>(q));
}
