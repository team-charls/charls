// SPDX-FileCopyrightText: © 2015 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <gtest/gtest.h>

#include <charls/jpegls_error.hpp>

#include "../src/jpeg_stream_writer.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace charls::test {

[[nodiscard]]
std::vector<std::byte> read_file(const char* filename);

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

[[nodiscard]]
inline std::vector<std::byte> create_test_spiff_header(const uint8_t high_version = 2, const uint8_t low_version = 0,
                                                        const bool end_of_directory = true,
                                                        const uint8_t component_count = 3)
{
    using std::byte;

    std::vector<byte> buffer;
    buffer.push_back(byte{0xFF});
    buffer.push_back(byte{0xD8}); // SOI.
    buffer.push_back(byte{0xFF});
    buffer.push_back(byte{0xE8}); // ApplicationData8
    buffer.push_back({});
    buffer.push_back(byte{32});

    // SPIFF identifier string.
    buffer.push_back(byte{'S'});
    buffer.push_back(byte{'P'});
    buffer.push_back(byte{'I'});
    buffer.push_back(byte{'F'});
    buffer.push_back(byte{'F'});
    buffer.push_back({});

    // Version
    buffer.push_back(byte{high_version});
    buffer.push_back(byte{low_version});

    buffer.push_back({}); // profile id
    buffer.push_back(byte{component_count});

    // Height
    buffer.push_back({});
    buffer.push_back({});
    buffer.push_back(byte{0x3});
    buffer.push_back(byte{0x20});

    // Width
    buffer.push_back({});
    buffer.push_back({});
    buffer.push_back(byte{0x2});
    buffer.push_back(byte{0x58});

    buffer.push_back(byte{10}); // color space
    buffer.push_back(byte{8});  // bits per sample
    buffer.push_back(byte{6});  // compression type, 6 = JPEG-LS
    buffer.push_back(byte{1});  // resolution units

    // vertical_resolution
    buffer.push_back({});
    buffer.push_back({});
    buffer.push_back({});
    buffer.push_back(byte{96});

    // header.horizontal_resolution = 1024
    buffer.push_back({});
    buffer.push_back({});
    buffer.push_back(byte{4});
    buffer.push_back({});

    const size_t spiff_header_size{buffer.size()};
    buffer.resize(buffer.size() + 100);

    jpeg_stream_writer writer;
    writer.destination({buffer.data() + spiff_header_size, buffer.size() - spiff_header_size});

    if (end_of_directory)
    {
        writer.write_spiff_end_of_directory_entry();
    }

    writer.write_start_of_frame_segment({600, 800, 8, 3});
    writer.write_start_of_scan_segment(1, 0, interleave_mode::none);

    return buffer;
}

} // namespace charls::test
