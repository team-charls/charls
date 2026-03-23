// SPDX-FileCopyrightText: © 2019 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "support.hpp"

#include <charls/charls.hpp>

#include <array>

using std::array;
using std::byte;

// ReSharper disable CppClangTidyClangDiagnosticNonnull

MSVC_WARNING_SUPPRESS(6387) // '_Param_(x)' could be '0': this does not adhere to the specification for the function.

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
#endif

namespace charls::test {

namespace {

void destroy_decoder(const charls_jpegls_decoder* decoder) noexcept
{
    charls_jpegls_decoder_destroy(decoder);
}

std::unique_ptr<charls_jpegls_decoder, void (*)(const charls_jpegls_decoder*)> get_initialized_decoder()
{
    static const auto source{read_file("data/t8c0e0.jls")};
    auto* const decoder{charls_jpegls_decoder_create()};
    auto error{charls_jpegls_decoder_set_source_buffer(decoder, source.data(), source.size())};
    EXPECT_EQ(jpegls_errc::success, error);
    error = charls_jpegls_decoder_read_header(decoder);
    EXPECT_EQ(jpegls_errc::success, error);

    return {decoder, destroy_decoder};
}

} // namespace


TEST(charls_jpegls_decoder_test, destroy_nullptr)
{
    charls_jpegls_decoder_destroy(nullptr);

    // No explicit test possible, code should remain stable.
    EXPECT_TRUE(true);
}

TEST(charls_jpegls_decoder_test, set_source_buffer_nullptr)
{
    constexpr array<byte, 10> buffer{};

    auto error{charls_jpegls_decoder_set_source_buffer(nullptr, buffer.data(), buffer.size())};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    auto* const decoder{charls_jpegls_decoder_create()};
    error = charls_jpegls_decoder_set_source_buffer(decoder, nullptr, buffer.size());
    charls_jpegls_decoder_destroy(decoder);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_decoder_test, read_spiff_header_nullptr)
{
    charls_spiff_header spiff_header{};
    int32_t header_found;
    auto error{charls_jpegls_decoder_read_spiff_header(nullptr, &spiff_header, &header_found)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    const auto source{read_file("data/t8c0e0.jls")};
    auto* decoder{charls_jpegls_decoder_create()};
    error = charls_jpegls_decoder_set_source_buffer(decoder, source.data(), source.size());
    EXPECT_EQ(jpegls_errc::success, error);
    error = charls_jpegls_decoder_read_spiff_header(decoder, nullptr, &header_found);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
    charls_jpegls_decoder_destroy(decoder);

    decoder = charls_jpegls_decoder_create();
    error = charls_jpegls_decoder_set_source_buffer(decoder, source.data(), source.size());
    EXPECT_EQ(jpegls_errc::success, error);
    error = charls_jpegls_decoder_read_spiff_header(decoder, &spiff_header, nullptr);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
    charls_jpegls_decoder_destroy(decoder);
}

TEST(charls_jpegls_decoder_test, read_header_nullptr)
{
    const auto error{charls_jpegls_decoder_read_header(nullptr)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_decoder_test, get_frame_info_nullptr)
{
    frame_info frame_info;
    auto error{charls_jpegls_decoder_get_frame_info(nullptr, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    const auto decoder{get_initialized_decoder()};
    error = charls_jpegls_decoder_get_frame_info(decoder.get(), nullptr);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_decoder_test, get_near_lossless_nullptr)
{
    int32_t near_lossless;
    auto error{charls_jpegls_decoder_get_near_lossless(nullptr, 0, &near_lossless)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    const auto decoder{get_initialized_decoder()};
    error = charls_jpegls_decoder_get_near_lossless(decoder.get(), 0, nullptr);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_decoder_test, get_interleave_mode_nullptr)
{
    interleave_mode interleave_mode;
    auto error{charls_jpegls_decoder_get_interleave_mode(nullptr, 0, &interleave_mode)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    const auto decoder{get_initialized_decoder()};
    error = charls_jpegls_decoder_get_interleave_mode(decoder.get(), 0, nullptr);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_decoder_test, get_preset_coding_parameters_nullptr)
{
    jpegls_pc_parameters preset_coding_parameters;
    auto error{charls_jpegls_decoder_get_preset_coding_parameters(nullptr, 0, &preset_coding_parameters)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    const auto decoder{get_initialized_decoder()};
    error = charls_jpegls_decoder_get_preset_coding_parameters(decoder.get(), 0, nullptr);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_decoder_test, get_color_transformation_nullptr)
{
    charls_color_transformation color_transformation;
    auto error{charls_jpegls_decoder_get_color_transformation(nullptr, &color_transformation)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    const auto decoder{get_initialized_decoder()};
    error = charls_jpegls_decoder_get_color_transformation(decoder.get(), nullptr);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_decoder_test, get_destination_size_nullptr)
{
    size_t destination_size_bytes;
    auto error{charls_jpegls_decoder_get_destination_size(nullptr, 0, &destination_size_bytes)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    const auto decoder{get_initialized_decoder()};
    error = charls_jpegls_decoder_get_destination_size(decoder.get(), 0, nullptr);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_decoder_test, decode_to_buffer_nullptr)
{
    array<byte, 5> buffer{};
    auto error{charls_jpegls_decoder_decode_to_buffer(nullptr, buffer.data(), buffer.size(), 0)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    auto* decoder{charls_jpegls_decoder_create()};
    error = charls_jpegls_decoder_decode_to_buffer(decoder, nullptr, buffer.size(), 0);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    charls_jpegls_decoder_destroy(decoder);
}

TEST(charls_jpegls_decoder_test, read_header_from_zero_size_buffer)
{
    auto* decoder{charls_jpegls_decoder_create()};
    auto error{charls_jpegls_decoder_set_source_buffer(decoder, nullptr, 0)};
    EXPECT_EQ(jpegls_errc::success, error);

    error = charls_jpegls_decoder_read_header(decoder);
    EXPECT_EQ(jpegls_errc::need_more_data, error);

    charls_jpegls_decoder_destroy(decoder);
}

TEST(charls_jpegls_decoder_test, decode_to_zero_size_buffer)
{
    const auto decoder{get_initialized_decoder()};

    const auto error{charls_jpegls_decoder_decode_to_buffer(decoder.get(), nullptr, 0, 0)};
    EXPECT_EQ(jpegls_errc::invalid_argument_size, error);
}

TEST(charls_jpegls_decoder_test, at_comment_nullptr)
{
    const auto error{charls_jpegls_decoder_at_comment(nullptr, nullptr, nullptr)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_decoder_test, at_application_data_nullptr)
{
    const auto error{charls_jpegls_decoder_at_application_data(nullptr, nullptr, nullptr)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_decoder_test, get_compressed_data_format_nullptr)
{
    charls_compressed_data_format compressed_data_format;
    auto error{charls_decoder_get_compressed_data_format(nullptr, &compressed_data_format)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    const auto decoder{get_initialized_decoder()};
    error = charls_decoder_get_compressed_data_format(decoder.get(), nullptr);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_decoder_test, charls_decoder_get_mapping_table_count_nullptr)
{
    int32_t count{7};
    auto error{charls_decoder_get_mapping_table_count(nullptr, &count)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
    EXPECT_EQ(7, count);

    const auto decoder{get_initialized_decoder()};
    size_t destination_size;
    error = charls_jpegls_decoder_get_destination_size(decoder.get(), 0, &destination_size);
    EXPECT_EQ(jpegls_errc::success, error);

    std::vector<byte> decoded_destination(destination_size);
    error = charls_jpegls_decoder_decode_to_buffer(decoder.get(), decoded_destination.data(), decoded_destination.size(), 0);
    EXPECT_EQ(jpegls_errc::success, error);

    error = charls_decoder_get_mapping_table_count(decoder.get(), nullptr);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_decoder_test, charls_decoder_get_mapping_table_info_nullptr)
{
    charls_mapping_table_info mapping_table_info{};
    auto error{charls_decoder_get_mapping_table_info(nullptr, 0, &mapping_table_info)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    const auto decoder{get_initialized_decoder()};
    size_t destination_size_bytes;
    error = charls_jpegls_decoder_get_destination_size(decoder.get(), 0, &destination_size_bytes);
    EXPECT_EQ(jpegls_errc::success, error);
    std::vector<byte> destination(destination_size_bytes);
    error = charls_jpegls_decoder_decode_to_buffer(decoder.get(), destination.data(), destination.size(), 0);
    EXPECT_EQ(jpegls_errc::success, error);

    error = charls_decoder_get_mapping_table_info(decoder.get(), 0, nullptr);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

} // namespace charls::test

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

MSVC_WARNING_UNSUPPRESS()
