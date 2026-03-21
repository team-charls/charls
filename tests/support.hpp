// SPDX-FileCopyrightText: © 2015 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <gtest/gtest.h>

#include <charls/jpegls_error.hpp>

#include "../test/portable_anymap_file.hpp"

#include <cstdint>
#include <cstring>
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
bool verify_encoded_bytes(const std::vector<std::byte>& uncompressed_source, const std::vector<std::byte>& encoded_source);

void test_compliance(const std::vector<std::byte>& encoded_source, const std::vector<std::byte>& uncompressed_source,
                     bool check_encode);

void decode_encode_file(const char* encoded_filename, const char* raw_filename, bool check_encode = true);

template<typename Functor>
void assert_expect_exception(const jpegls_errc error_value, Functor functor)
{
    try
    {
        functor();
    }
    catch (const jpegls_error& error)
    {
        EXPECT_EQ(error_value, error.code());
        ASSERT_NE(nullptr, error.what());
        EXPECT_GT(strlen(error.what()), size_t{});
        return;
    }
    catch (...)
    {
        throw;
    }

    ADD_FAILURE() << "Expected jpegls_error exception was not thrown";
}

} // namespace charls::test
