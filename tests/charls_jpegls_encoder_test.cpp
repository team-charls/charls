// SPDX-FileCopyrightText: © 2019 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>

#include "support.hpp"

#include <charls/charls.hpp>

#include <array>

using std::array;
using std::byte;
using std::ignore;
using std::vector;

////MSVC_WARNING_SUPPRESS(6387) // '_Param_(x)' could be '0': this does not adhere to the specification for the function.

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
#endif

namespace charls::test {

TEST(charls_jpegls_encoder_test, destroy_nullptr)
{
    charls_jpegls_encoder_destroy(nullptr);

    // No explicit test possible, code should remain stable.
    EXPECT_TRUE(true);
}

TEST(charls_jpegls_encoder_test, set_destination_buffer_nullptr)
{
    array<byte, 10> buffer{};
    auto error{charls_jpegls_encoder_set_destination_buffer(nullptr, buffer.data(), buffer.size())};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    auto* const encoder{charls_jpegls_encoder_create()};
    error = charls_jpegls_encoder_set_destination_buffer(encoder, nullptr, buffer.size());
    charls_jpegls_encoder_destroy(encoder);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, set_frame_info_buffer_nullptr)
{
    constexpr charls_frame_info frame_info{};
    auto error{charls_jpegls_encoder_set_frame_info(nullptr, &frame_info)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    auto* const encoder{charls_jpegls_encoder_create()};
    error = charls_jpegls_encoder_set_frame_info(encoder, nullptr);
    charls_jpegls_encoder_destroy(encoder);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, set_near_lossless_nullptr)
{
    const auto error{charls_jpegls_encoder_set_near_lossless(nullptr, 1)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, set_interleave_mode_nullptr)
{
    const auto error{charls_jpegls_encoder_set_interleave_mode(nullptr, charls_interleave_mode::line)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, set_preset_coding_parameters_nullptr)
{
    constexpr jpegls_pc_parameters parameters{};
    auto error{charls_jpegls_encoder_set_preset_coding_parameters(nullptr, &parameters)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    auto* const encoder{charls_jpegls_encoder_create()};
    error = charls_jpegls_encoder_set_preset_coding_parameters(encoder, nullptr);
    charls_jpegls_encoder_destroy(encoder);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, set_color_transformation_nullptr)
{
    const auto error{charls_jpegls_encoder_set_color_transformation(nullptr, charls_color_transformation::hp1)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, charls_jpegls_encoder_set_table_id_nullptr)
{
    const auto error{charls_jpegls_encoder_set_mapping_table_id(nullptr, 0, 0)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, get_estimated_destination_size_nullptr)
{
    size_t size_in_bytes{};
    auto error{charls_jpegls_encoder_get_estimated_destination_size(nullptr, &size_in_bytes)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    charls_jpegls_encoder* const encoder{charls_jpegls_encoder_create()};

    constexpr charls_frame_info frame_info{1, 1, 2, 1};
    error = charls_jpegls_encoder_set_frame_info(encoder, &frame_info);
    EXPECT_EQ(jpegls_errc::success, error);

    error = charls_jpegls_encoder_get_estimated_destination_size(encoder, nullptr);
    charls_jpegls_encoder_destroy(encoder);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, get_bytes_written_nullptr)
{
    size_t bytes_written{};
    auto error{charls_jpegls_encoder_get_bytes_written(nullptr, &bytes_written)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    charls_jpegls_encoder const* const encoder{charls_jpegls_encoder_create()};
    error = charls_jpegls_encoder_get_bytes_written(encoder, nullptr);
    charls_jpegls_encoder_destroy(encoder);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, encode_from_buffer_nullptr)
{
    constexpr array<byte, 10> source_buffer{};
    auto error{charls_jpegls_encoder_encode_from_buffer(nullptr, source_buffer.data(), source_buffer.size(), 0)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    auto* const encoder{charls_jpegls_encoder_create()};
    error = charls_jpegls_encoder_encode_from_buffer(encoder, nullptr, source_buffer.size(), 0);
    charls_jpegls_encoder_destroy(encoder);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, encode_components_from_buffer_nullptr)
{
    constexpr array<byte, 10> source_buffer{};
    auto error{
        charls_jpegls_encoder_encode_components_from_buffer(nullptr, source_buffer.data(), source_buffer.size(), 1, 0)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    auto* const encoder{charls_jpegls_encoder_create()};
    error = charls_jpegls_encoder_encode_components_from_buffer(encoder, nullptr, source_buffer.size(), 1, 0);
    charls_jpegls_encoder_destroy(encoder);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, write_spiff_header_nullptr)
{
    constexpr charls_spiff_header spiff_header{};
    auto error{charls_jpegls_encoder_write_spiff_header(nullptr, &spiff_header)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    auto* const encoder{charls_jpegls_encoder_create()};
    error = charls_jpegls_encoder_write_spiff_header(encoder, nullptr);
    charls_jpegls_encoder_destroy(encoder);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, write_standard_spiff_header_nullptr)
{
    const auto error{charls_jpegls_encoder_write_standard_spiff_header(
        nullptr, charls_spiff_color_space::cie_lab, charls_spiff_resolution_units::dots_per_centimeter, 1, 1)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, write_spiff_entry_nullptr)
{
    constexpr array<byte, 10> entry_data{};
    auto error{charls_jpegls_encoder_write_spiff_entry(nullptr, 5, entry_data.data(), entry_data.size())};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);

    auto* const encoder{charls_jpegls_encoder_create()};
    error = charls_jpegls_encoder_write_spiff_entry(encoder, 5, nullptr, sizeof(entry_data));
    charls_jpegls_encoder_destroy(encoder);
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, write_spiff_end_of_directory_entry_before_header_throws)
{
    const auto error{charls_jpegls_encoder_write_spiff_end_of_directory_entry(nullptr)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, write_comment_nullptr)
{
    constexpr array<byte, 10> buffer{};
    const auto error{charls_jpegls_encoder_write_comment(nullptr, buffer.data(), buffer.size())};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, write_application_data_nullptr)
{
    constexpr array<byte, 10> buffer{};
    const auto error{charls_jpegls_encoder_write_application_data(nullptr, 0, buffer.data(), buffer.size())};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, write_table_data_nullptr)
{
    constexpr array<byte, 10> buffer{};
    const auto error{charls_jpegls_encoder_write_mapping_table(nullptr, 1, 1, buffer.data(), buffer.size())};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, rewind_nullptr)
{
    const auto error{charls_jpegls_encoder_rewind(nullptr)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, encode_to_zero_size_buffer)
{
    auto* encoder{charls_jpegls_encoder_create()};
    auto error{charls_jpegls_encoder_set_destination_buffer(encoder, nullptr, 0)};
    EXPECT_EQ(jpegls_errc::success, error);

    constexpr charls_frame_info frame_info{1, 1, 2, 1};
    error = charls_jpegls_encoder_set_frame_info(encoder, &frame_info);
    EXPECT_EQ(jpegls_errc::success, error);

    constexpr array<byte, 10> buffer{};
    error = charls_jpegls_encoder_encode_from_buffer(encoder, buffer.data(), buffer.size(), 0);
    EXPECT_EQ(jpegls_errc::destination_too_small, error);

    charls_jpegls_encoder_destroy(encoder);
}

TEST(charls_jpegls_encoder_test, encode_from_zero_size_buffer)
{
    auto* encoder{charls_jpegls_encoder_create()};

    array<byte, 10> buffer{};
    auto error{charls_jpegls_encoder_set_destination_buffer(encoder, buffer.data(), buffer.size())};
    EXPECT_EQ(jpegls_errc::success, error);

    constexpr charls_frame_info frame_info{1, 1, 2, 1};
    error = charls_jpegls_encoder_set_frame_info(encoder, &frame_info);
    EXPECT_EQ(jpegls_errc::success, error);

    error = charls_jpegls_encoder_encode_from_buffer(encoder, nullptr, 0, 0);
    EXPECT_EQ(jpegls_errc::invalid_argument_size, error);

    charls_jpegls_encoder_destroy(encoder);
}

TEST(charls_jpegls_encoder_test, create_tables_only_null_ptr)
{
    const auto error{charls_jpegls_encoder_create_abbreviated_format(nullptr)};
    EXPECT_EQ(jpegls_errc::invalid_argument, error);
}

TEST(charls_jpegls_encoder_test, encode_non_8_or_16_bit_with_color_transformation_throws)
{
    constexpr frame_info frame_info{2, 1, 10, 3};
    jpegls_encoder encoder;

    vector<byte> destination(40);
    encoder.destination(destination).frame_info(frame_info).color_transformation(color_transformation::hp3);
    const vector<byte> source(20);
    assert_expect_exception(jpegls_errc::invalid_argument_color_transformation,
                            [&encoder, &source] { ignore = encoder.encode(source); });
}

TEST(charls_jpegls_encoder_test, encode_non_3_components_that_is_not_supported_throws)
{
    constexpr frame_info frame_info{2, 1, 8, 4};
    jpegls_encoder encoder;

    vector<byte> destination(40);
    encoder.destination(destination).frame_info(frame_info).color_transformation(color_transformation::hp3);
    const vector<byte> source(20);
    assert_expect_exception(jpegls_errc::invalid_argument_color_transformation,
                            [&encoder, &source] { ignore = encoder.encode(source); });
}

} // namespace charls::test

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

////MSVC_WARNING_UNSUPPRESS()
