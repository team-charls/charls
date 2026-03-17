// SPDX-FileCopyrightText: © 2015 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>

#include "jpeg_test_stream_writer.hpp"
#include "test_util.hpp"

#include "../src/jpeg_stream_reader.hpp"
#include "../src/jpeg_stream_writer.hpp"

#include <array>
#include <cstdint>
#include <vector>

using std::array;
using std::byte;
using std::numeric_limits;
using std::vector;

namespace charls::test {

namespace {

void read_spiff_header(const uint8_t low_version)
{
    auto buffer{create_test_spiff_header(2, low_version)};
    jpeg_stream_reader reader;
    reader.source({buffer.data(), buffer.size()});

    spiff_header spiff_header{};
    bool spiff_header_found{};

    reader.read_header(&spiff_header, &spiff_header_found);

    EXPECT_TRUE(spiff_header_found);
    EXPECT_EQ(static_cast<int32_t>(spiff_profile_id::none), static_cast<int32_t>(spiff_header.profile_id));
    EXPECT_EQ(3, spiff_header.component_count);
    EXPECT_EQ(800U, spiff_header.height);
    EXPECT_EQ(600U, spiff_header.width);
    EXPECT_EQ(static_cast<int32_t>(spiff_color_space::rgb), static_cast<int32_t>(spiff_header.color_space));
    EXPECT_EQ(8, spiff_header.bits_per_sample);
    EXPECT_EQ(static_cast<int32_t>(spiff_compression_type::jpeg_ls),
              static_cast<int32_t>(spiff_header.compression_type));
    EXPECT_EQ(static_cast<int32_t>(spiff_resolution_units::dots_per_inch),
              static_cast<int32_t>(spiff_header.resolution_units));
    EXPECT_EQ(96U, spiff_header.vertical_resolution);
    EXPECT_EQ(1024U, spiff_header.horizontal_resolution);
}

void read_header_with_application_data(const uint8_t data_number)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();

    writer.write_byte(byte{0xFF});
    writer.write_byte(static_cast<byte>(0xE0 + data_number));
    writer.write_byte(byte{0x00});
    writer.write_byte(byte{0x02});

    writer.write_start_of_frame_segment(1, 1, 2, 1);
    writer.write_start_of_scan_segment(0, 1, 1, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    reader.read_header(); // if it doesn't throw test is passed.
}

void read_hp_color_transform(const color_transformation transformation)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_hp_color_transform_segment(transformation);
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();

    EXPECT_EQ(transformation, reader.parameters().transformation);
}

void read_hp_color_transform_unsupported_throws(const color_transformation transformation, const jpegls_errc error)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_hp_color_transform_segment(transformation);
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(error, [&reader] { reader.read_header(); });
}

void read_header_incorrect_interleave_in_sos_for_single_component_throws(const interleave_mode mode)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 1);
    writer.write_start_of_scan_segment(0, 1, 0, mode);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_interleave_mode, [&reader] { reader.read_header(); });
}

void read_header_with_jpeg_ls_preset_parameter_with_extended_id_throws(const byte id)
{
    const array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF},
                       byte{0xF8}, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                       byte{0x00}, byte{0x03}, id};

    jpeg_stream_reader reader;
    reader.source({buffer.data(), buffer.size()});

    assert_expect_exception(jpegls_errc::jpegls_preset_extended_parameter_type_not_supported,
                            [&reader] { reader.read_header(); });
}

} // namespace

TEST(jpeg_stream_reader_test, read_header_from_to_small_input_buffer_throws)
{
    array<byte, 1> buffer{};
    jpeg_stream_reader reader;
    reader.source({buffer.data(), 0});

    assert_expect_exception(jpegls_errc::need_more_data, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_from_buffer_preceded_with_fill_bytes)
{
    constexpr byte extra_start_byte{0xFF};
    jpeg_test_stream_writer writer;

    writer.write_byte(extra_start_byte);
    writer.write_start_of_image();

    writer.write_byte(extra_start_byte);
    writer.write_start_of_frame_segment(1, 1, 2, 1);

    writer.write_byte(extra_start_byte);
    writer.write_start_of_scan_segment(0, 1, 1, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    reader.read_header(); // if it doesn't throw test is passed.
}

TEST(jpeg_stream_reader_test, read_header_from_buffer_not_starting_with_ff_throws)
{
    constexpr array buffer{byte{0x0F}, byte{0xFF}, byte{0xD8},
                           byte{0xFF}, byte{0xFF}, byte{0xDA}}; // 0xDA = SOS: Marks the start of scan.

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::jpeg_marker_start_byte_not_found, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_from_buffer_not_starting_with_soi_throws)
{
    constexpr array buffer{byte{0xFF}, byte{0xD7}}; // 0xD7 = RST7: Restart marker (not a SOI marker).

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::start_of_image_marker_not_found, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_application_data)
{
    for (uint8_t i{}; i != 16; ++i)
    {
        read_header_with_application_data(i);
    }
}

TEST(jpeg_stream_reader_test, read_header_with_jpegls_extended_frame_throws)
{
    constexpr array<byte, 6> buffer{
        byte{0xFF}, byte{0xD8}, byte{0xFF},
        byte{0xF9}}; // 0xF9 = SOF_57: Marks the start of a JPEG-LS extended (ISO/IEC 14495-2) encoded frame.

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::encoding_not_supported, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_jpegls_preset_parameter_segment)
{
    vector<byte> source(100);
    jpeg_stream_writer writer;
    writer.destination({source.data(), source.size()});
    writer.write_start_of_image();

    constexpr jpegls_pc_parameters presets{1, 2, 3, 4, 5};
    writer.write_jpegls_preset_parameters_segment(presets);
    writer.write_start_of_frame_segment({1, 1, 2, 1});
    writer.write_start_of_scan_segment(1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({source.data(), source.size()});

    reader.read_header();
    const auto& [maximum_sample_value, threshold1, threshold2, threshold3,
                 reset_value]{reader.preset_coding_parameters()};

    EXPECT_EQ(presets.maximum_sample_value, maximum_sample_value);
    EXPECT_EQ(presets.reset_value, reset_value);
    EXPECT_EQ(presets.threshold1, threshold1);
    EXPECT_EQ(presets.threshold2, threshold2);
    EXPECT_EQ(presets.threshold3, threshold3);
}

TEST(jpeg_stream_reader_test, read_header_with_too_small_jpegls_preset_parameter_segment_throws)
{
    constexpr array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF},
                           byte{0xF8}, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                           byte{0x00}, byte{0x02}, byte{0x01}};

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_too_small_jpegls_preset_parameter_segment_with_coding_parameters_throws)
{
    constexpr array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF},
                           byte{0xF8}, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                           byte{0x00}, byte{0x0A}, byte{0x01}};

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_too_large_jpegls_preset_parameter_segment_with_coding_parameters_throws)
{
    constexpr array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF},
                           byte{0xF8}, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                           byte{0x00}, byte{0x0C}, byte{0x01}};

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_jpegls_preset_parameter_with_extended_id_throws)
{
    constexpr array ids{byte{0x5}, byte{0x6}, byte{0x7}, byte{0x8}, byte{0x9}, byte{0xA}, byte{0xC}, byte{0xD}};

    for (const auto id : ids)
    {
        read_header_with_jpeg_ls_preset_parameter_with_extended_id_throws(id);
    }
}

TEST(jpeg_stream_reader_test, read_header_with_jpegls_preset_parameter_with_invalid_id_throws)
{
    constexpr array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF},
                           byte{0xF8}, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                           byte{0x00}, byte{0x03}, byte{0xE}};

    jpeg_stream_reader reader;
    reader.source({buffer.data(), buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_jpegls_preset_parameter_type, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_too_small_segment_size_throws)
{
    constexpr array buffer{
        byte{0xFF}, byte{0xD8}, byte{0xFF}, byte{0xF7},  // SOF_55: Marks the start of JPEG-LS extended scan.
        byte{0x00}, byte{0x01}, byte{0xFF}, byte{0xDA}}; // SOS: Marks the start of scan.

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_too_small_start_of_frame_throws)
{
    constexpr array buffer{
        byte{0xFF}, byte{0xD8}, byte{0xFF}, byte{0xF7}, // SOF_55: Marks the start of JPEG-LS extended scan.
        byte{0x00}, byte{0x06}, byte{2},    byte{2},    byte{2}, byte{2}, byte{2}, byte{2}, byte{1}};

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_too_small_start_of_frame_in_component_info_throws)
{
    constexpr array buffer{
        byte{0xFF}, byte{0xD8}, byte{0xFF}, byte{0xF7}, // SOF_55: Marks the start of JPEG-LS extended scan.
        byte{0x00}, byte{0x08}, byte{2},    byte{2},    byte{2}, byte{2}, byte{2}, byte{2}, byte{1}};

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_too_large_start_of_frame_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.buffer.push_back({});
    writer.buffer[5] = static_cast<byte>(static_cast<uint8_t>(writer.buffer[5]) + 1);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_invalid_bits_per_sample_in_min_sof_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 1, 3);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_bits_per_sample, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_invalid_bits_per_sample_in_max_sof_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 17, 3);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_bits_per_sample, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_unknown_marker_code_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_marker(static_cast<jpeg_marker_code>(0xFA));

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::unknown_jpeg_marker_found, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_sos_before_sof_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_scan_segment(0, 1, 128, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::unexpected_start_of_scan_marker, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_extra_sof_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_start_of_frame_segment(512, 512, 8, 3);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::duplicate_start_of_frame_marker, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_too_large_near_lossless_in_sos_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_start_of_scan_segment(0, 1, 128, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_near_lossless, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_too_large_near_lossless_in_sos_throws2)
{
    constexpr jpegls_pc_parameters preset_coding_parameters{200, 0, 0, 0, 0};

    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_jpegls_preset_parameters_segment(preset_coding_parameters);
    writer.write_start_of_frame_segment(512, 512, 8, 3);

    constexpr int bad_near_lossless{(200 / 2) + 1};
    writer.write_start_of_scan_segment(0, 1, bad_near_lossless, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_near_lossless, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_line_interleave_in_sos_for_single_component_throws)
{
    read_header_incorrect_interleave_in_sos_for_single_component_throws(interleave_mode::line);
}

TEST(jpeg_stream_reader_test, read_header_sample_interleave_in_sos_for_single_component_throws)
{
    read_header_incorrect_interleave_in_sos_for_single_component_throws(interleave_mode::sample);
}

TEST(jpeg_stream_reader_test, read_header_with_duplicate_component_id_in_start_of_frame_segment_throws)
{
    jpeg_test_stream_writer writer;
    writer.component_id_override = 7;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 3);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::duplicate_component_id_in_sof_segment, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_too_many_components_in_start_of_frame_segment_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 256);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_component_count, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_no_components_in_start_of_frame_segment_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 0);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_component_count, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_too_many_components_in_start_of_scan_segment_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 1);
    writer.write_start_of_scan_segment(0, 2, 0, interleave_mode::sample);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_component_count, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_no_components_in_start_of_scan_segment_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 1);
    writer.write_start_of_scan_segment(0, 0, 0, interleave_mode::sample);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_component_count, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_more_than_max_components_in_start_of_scan_segment_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 5);
    writer.write_start_of_scan_segment(0, 5, 0, interleave_mode::sample);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_component_count, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_unknown_component_id_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 1);
    writer.write_start_of_scan_segment(4, 1, 1, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::unknown_component_id, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_unknown_component_id_but_all_defaults_is_ignored)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 1);
    writer.write_start_of_scan_segment(4, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    reader.read_header();

    EXPECT_EQ(size_t{1}, reader.component_count());
}

TEST(jpeg_stream_reader_test, read_header_with_too_small_start_of_scan_throws)
{
    constexpr array buffer{byte{0xFF}, byte{0xD8},
                           byte{0xFF}, byte{0xF7}, // SOF_55: Marks the start of JPEG-LS extended scan.
                           byte{},     byte{0x08}, // size
                           byte{0x08},             // bits per sample
                           byte{},     byte{0x01}, // width
                           byte{},     byte{0x01}, // height
                           byte{0x01},             // component count
                           byte{0xFF}, byte{0xDA}, // SOS
                           byte{},     byte{0x03}};

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_too_small_start_of_scan_component_count_throws)
{
    constexpr array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF},
                           byte{0xF7},             // SOF_55: Marks the start of JPEG-LS extended scan.
                           byte{0x00}, byte{0x08}, // size
                           byte{0x08},             // bits per sample
                           byte{0x00}, byte{0x01}, // width
                           byte{0x00}, byte{0x01}, // height
                           byte{0x01},             // component count
                           byte{0xFF}, byte{0xDA}, // SOS
                           byte{0x00}, byte{0x07}, byte{0x01}};

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_directly_end_of_image_throws)
{
    constexpr array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF}, byte{0xD9}}; // 0xD9 = EOI

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::unexpected_end_of_image_marker, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_duplicate_start_of_image_throws)
{
    constexpr array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF}, byte{0xD8}}; // 0xD8 = SOI.

    jpeg_stream_reader reader;
    reader.source(buffer);

    assert_expect_exception(jpegls_errc::duplicate_start_of_image_marker, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_spiff_header)
{
    read_spiff_header(0);
}

TEST(jpeg_stream_reader_test, read_spiff_header_low_version_newer)
{
    read_spiff_header(1);
}

TEST(jpeg_stream_reader_test, read_spiff_header_high_version_to_new)
{
    const auto buffer{create_test_spiff_header(3)};
    jpeg_stream_reader reader;
    reader.source({buffer.data(), buffer.size()});

    spiff_header spiff_header{};
    bool spiff_header_found{};

    reader.read_header(&spiff_header, &spiff_header_found);

    EXPECT_FALSE(spiff_header_found);
}

TEST(jpeg_stream_reader_test, read_spiff_header_without_end_of_directory)
{
    const auto buffer{create_test_spiff_header(2, 0, false)};
    jpeg_stream_reader reader;
    reader.source({buffer.data(), buffer.size()});

    spiff_header spiff_header{};
    bool spiff_header_found{};

    reader.read_header(&spiff_header, &spiff_header_found);
    EXPECT_TRUE(spiff_header_found);

    assert_expect_exception(jpegls_errc::missing_end_of_spiff_directory, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_header_with_define_restart_interval_16_bit)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_define_restart_interval(numeric_limits<uint16_t>::max() - 5U, 2);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();

    EXPECT_EQ(static_cast<uint32_t>(numeric_limits<uint16_t>::max() - 5), reader.parameters().restart_interval);
}

TEST(jpeg_stream_reader_test, read_header_with_define_restart_interval_24_bit)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_define_restart_interval(numeric_limits<uint16_t>::max() + 5U, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();

    EXPECT_EQ(static_cast<uint32_t>(numeric_limits<uint16_t>::max() + 5), reader.parameters().restart_interval);
}

TEST(jpeg_stream_reader_test, read_header_with_define_restart_interval_32_bit)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_define_restart_interval(numeric_limits<uint32_t>::max() - 7, 4);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();

    EXPECT_EQ(numeric_limits<uint32_t>::max() - 7, reader.parameters().restart_interval);
}

TEST(jpeg_stream_reader_test, read_header_with_2_define_restart_intervals)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_define_restart_interval(numeric_limits<uint32_t>::max(), 4);
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_define_restart_interval(0, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();

    EXPECT_EQ(0U, reader.parameters().restart_interval);
}

TEST(jpeg_stream_reader_test, read_header_with_bad_define_restart_interval)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 3);

    constexpr array<byte, 1> buffer{};
    writer.write_segment(jpeg_marker_code::define_restart_interval, buffer.data(), buffer.size());
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_jpegls_stream_with_restart_marker_outside_entropy_data)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_restart_marker(0);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::unexpected_restart_marker, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_comment)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_segment(jpeg_marker_code::comment, "hello", 5);
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    struct callback_output
    {
        const void* data{};
        size_t size{};
    };
    callback_output actual;

    reader.at_comment({[](const void* data, const size_t size, void* user_context) noexcept -> int32_t {
                           auto* actual_output{static_cast<callback_output*>(user_context)};
                           actual_output->data = data;
                           actual_output->size = size;
                           return 0;
                       },
                       &actual});

    reader.read_header();

    EXPECT_EQ(size_t{5}, actual.size);
    EXPECT_TRUE(memcmp("hello", actual.data, actual.size) == 0);
}

TEST(jpeg_stream_reader_test, read_empty_comment)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_segment(jpeg_marker_code::comment, "", 0);
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    struct callback_output
    {
        const void* data{};
        size_t size{};
    };
    callback_output actual;

    reader.at_comment({[](const void* data, const size_t size, void* user_context) noexcept -> int32_t {
                           auto* actual_output{static_cast<callback_output*>(user_context)};
                           actual_output->data = data;
                           actual_output->size = size;
                           return 0;
                       },
                       &actual});

    reader.read_header();

    EXPECT_EQ(size_t{}, actual.size);
    EXPECT_EQ(nullptr, actual.data);
}

TEST(jpeg_stream_reader_test, read_comment_from_too_small_buffer_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_segment(jpeg_marker_code::comment, "", 0);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size() - 1});

    bool called{};
    reader.at_comment({[](const void*, const size_t, void* user_context) noexcept -> int32_t {
                           auto* actual_called{static_cast<bool*>(user_context)};
                           *actual_called = true;
                           return 0;
                       },
                       &called});

    assert_expect_exception(jpegls_errc::need_more_data, [&reader] { reader.read_header(); });
    EXPECT_FALSE(called);
}

TEST(jpeg_stream_reader_test, read_application_data)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_segment(jpeg_marker_code::application_data8, "hello", 5);
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    struct callback_output
    {
        int32_t id{};
        const void* data{};
        size_t size{};
    };
    callback_output actual;

    reader.at_application_data(
        {[](const int32_t id, const void* data, const size_t size, void* user_context) noexcept -> int32_t {
             auto* actual_output{static_cast<callback_output*>(user_context)};
             actual_output->id = id;
             actual_output->data = data;
             actual_output->size = size;
             return 0;
         },
         &actual});

    reader.read_header();

    EXPECT_EQ(8, actual.id);
    EXPECT_EQ(size_t{5}, actual.size);
    EXPECT_TRUE(memcmp("hello", actual.data, actual.size) == 0);
}

TEST(jpeg_stream_reader_test, read_empty_application_data)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_segment(jpeg_marker_code::application_data15, "", 0);
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    struct callback_output
    {
        int32_t id{};
        const void* data{};
        size_t size{};
    };
    callback_output actual;

    reader.at_application_data(
        {[](const int32_t id, const void* data, const size_t size, void* user_context) noexcept -> int32_t {
             auto* actual_output{static_cast<callback_output*>(user_context)};
             actual_output->id = id;
             actual_output->data = data;
             actual_output->size = size;
             return 0;
         },
         &actual});

    reader.read_header();

    EXPECT_EQ(15, actual.id);
    EXPECT_EQ(size_t{}, actual.size);
    EXPECT_EQ(nullptr, actual.data);
}

TEST(jpeg_stream_reader_test, read_application_data_from_too_small_buffer_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_segment(jpeg_marker_code::application_data14, "", 0);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size() - 1});

    bool called{};
    reader.at_application_data({[](int32_t, const void*, const size_t, void* user_context) noexcept -> int32_t {
                                    auto* actual_called{static_cast<bool*>(user_context)};
                                    *actual_called = true;
                                    return 0;
                                },
                                &called});

    assert_expect_exception(jpegls_errc::need_more_data, [&reader] { reader.read_header(); });
    EXPECT_FALSE(called);
}

TEST(jpeg_stream_reader_test, read_hp_color_transform)
{
    read_hp_color_transform(color_transformation::none);
    read_hp_color_transform(color_transformation::hp1);
    read_hp_color_transform(color_transformation::hp2);
    read_hp_color_transform(color_transformation::hp3);
}

TEST(jpeg_stream_reader_test, read_hp_color_transform_unsupported_throws)
{
    read_hp_color_transform_unsupported_throws(static_cast<color_transformation>(4),
                                               jpegls_errc::color_transform_not_supported);
    read_hp_color_transform_unsupported_throws(static_cast<color_transformation>(5),
                                               jpegls_errc::color_transform_not_supported);
    read_hp_color_transform_unsupported_throws(static_cast<color_transformation>(6),
                                               jpegls_errc::invalid_parameter_color_transformation);
}

TEST(jpeg_stream_reader_test, read_hp_color_transform_no_color_segment_present)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();

    EXPECT_EQ(color_transformation::none, reader.parameters().transformation);
}

TEST(jpeg_stream_reader_test, read_hp_color_transform_two_color_segments_present)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_hp_color_transform_segment(color_transformation::hp1);
    writer.write_start_of_frame_segment(512, 512, 8, 3);
    writer.write_hp_color_transform_segment(color_transformation::hp2);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();

    EXPECT_EQ(color_transformation::hp2, reader.parameters().transformation);
}

TEST(jpeg_stream_reader_test, read_end_of_image) // NOLINT
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 1, 8, 1);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);
    writer.write_byte(byte{0x80});
    writer.write_marker(jpeg_marker_code::end_of_image);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();
    reader.advance_position(1);

    reader.read_end_of_image(); // if it doesn't throw test is passed.
}

TEST(jpeg_stream_reader_test, read_end_of_image_with_zero_byte_padding) // NOLINT
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 1, 8, 1);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);
    writer.write_byte(byte{0x80});
    writer.write_byte(byte{0x00}); // zero byte padding after the entropy coded data, before the EOI marker.
    writer.write_marker(jpeg_marker_code::end_of_image);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();
    reader.advance_position(1);

    reader.read_end_of_image(); // if it doesn't throw test is passed.
}

TEST(jpeg_stream_reader_test, read_end_of_image_with_ffff_byte_padding) // NOLINT
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 1, 8, 1);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);
    writer.write_byte(byte{0x80});
    writer.write_byte(byte{0xFF});
    writer.write_byte(byte{0xFF});
    writer.write_marker(jpeg_marker_code::end_of_image);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();
    reader.advance_position(1);

    reader.read_end_of_image(); // if it doesn't throw test is passed.
}

TEST(jpeg_stream_reader_test, read_end_of_image_with_non_zero_byte_padding_throws) // NOLINT
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 1, 8, 1);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);
    writer.write_byte(byte{0x80});
    writer.write_byte(byte{0x01}); // nonzero byte padding after the entropy coded data, before the EOI marker.
    writer.write_marker(jpeg_marker_code::end_of_image);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();
    reader.advance_position(1);

    assert_expect_exception(jpegls_errc::end_of_image_marker_not_found, [&reader] { reader.read_end_of_image(); });
}

TEST(jpeg_stream_reader_test, read_end_of_image_bad_marker_throws) // NOLINT
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 1, 8, 1);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);
    writer.write_byte(byte{0x80});
    writer.write_marker(jpeg_marker_code::start_of_image);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();
    reader.advance_position(1);

    assert_expect_exception(jpegls_errc::end_of_image_marker_not_found, [&reader] { reader.read_end_of_image(); });
}

TEST(jpeg_stream_reader_test, read_end_of_image_00d9_throws) // NOLINT
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 1, 8, 1);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);
    writer.write_byte(byte{0x0});
    writer.write_byte(byte{0x00}); // Write instead of 0xFFD9 the value 0x00D9
    writer.write_byte(byte{0xD9});

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();
    reader.advance_position(1);

    assert_expect_exception(jpegls_errc::end_of_image_marker_not_found, [&reader] { reader.read_end_of_image(); });
}

TEST(jpeg_stream_reader_test, read_mapping_table)
{
    vector<byte> source(100);
    jpeg_stream_writer writer;
    writer.destination({source.data(), source.size()});
    writer.write_start_of_image();

    constexpr array table_data_expected{byte{2}};

    writer.write_jpegls_preset_parameters_segment(1, 1, table_data_expected);
    writer.write_start_of_frame_segment({1, 1, 2, 1});
    writer.write_start_of_scan_segment(1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({source.data(), source.size()});

    reader.read_header();

    EXPECT_EQ(size_t{1}, reader.mapping_table_count());
    EXPECT_EQ(0, reader.find_mapping_table_index(1));

    const auto info{reader.get_mapping_table_info(0)};
    EXPECT_EQ(int32_t{1}, info.table_id);
    EXPECT_EQ(int32_t{1}, info.entry_size);
    EXPECT_EQ(uint32_t{1}, info.data_size);

    vector<byte> table_data(1);
    reader.get_mapping_table_data(0, {table_data.data(), table_data.size()});
    EXPECT_EQ(byte{2}, table_data[0]);
}

TEST(jpeg_stream_reader_test, read_mapping_table_too_small_buffer_throws)
{
    vector<byte> source(100);
    jpeg_stream_writer writer;
    writer.destination({source.data(), source.size()});
    writer.write_start_of_image();

    constexpr array table_data_expected{byte{2}, byte{3}};

    writer.write_jpegls_preset_parameters_segment(1, 1, table_data_expected);
    writer.write_start_of_frame_segment({1, 1, 2, 1});
    writer.write_start_of_scan_segment(1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({source.data(), source.size()});
    reader.read_header();

    assert_expect_exception(jpegls_errc::destination_too_small, [&reader] {
        vector<byte> table_data(1);
        reader.get_mapping_table_data(0, {table_data.data(), table_data.size()});
    });
}

TEST(jpeg_stream_reader_test, mapping_table_count_is_zero_at_start)
{
    const jpeg_stream_reader reader;

    const auto count{reader.mapping_table_count()};

    EXPECT_EQ(size_t{}, count);
}

TEST(jpeg_stream_reader_test, mapping_table_count_after_read_header)
{
    const std::vector<std::byte> table_data(255);
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_jpegls_preset_parameters_segment(1, 1, table_data, false);
    writer.write_start_of_frame_segment(1, 1, 8, 3);
    writer.write_jpegls_preset_parameters_segment(2, 1, table_data, false);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    reader.read_header();
    const auto count{reader.mapping_table_count()};

    EXPECT_EQ(size_t{2}, count);
}

TEST(jpeg_stream_reader_test, mapping_table_count_after_read_header_after_frame)
{
    const vector<std::byte> table_data(255);
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 1, 8, 3);
    writer.write_jpegls_preset_parameters_segment(1, 1, table_data, false);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();
    const auto count{reader.mapping_table_count()};

    EXPECT_EQ(size_t{1}, count);
}

TEST(jpeg_stream_reader_test, mapping_table_count_after_read_header_before_frame)
{
    const vector<std::byte> table_data(255);
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_jpegls_preset_parameters_segment(1, 1, table_data, false);
    writer.write_start_of_frame_segment(1, 1, 8, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});
    reader.read_header();
    const auto count{reader.mapping_table_count()};

    EXPECT_EQ(size_t{1}, count);
}

TEST(jpeg_stream_reader_test, read_mapping_table_continuation)
{
    constexpr size_t table_size{100000};
    vector<byte> source(table_size + 100);
    jpeg_stream_writer writer;
    writer.destination({source.data(), source.size()});
    writer.write_start_of_image();

    vector<byte> table_data_expected(table_size);
    table_data_expected[0] = byte{7};
    table_data_expected[table_size - 1] = byte{8};

    writer.write_jpegls_preset_parameters_segment(1, 1, {table_data_expected.data(), table_data_expected.size()});
    writer.write_start_of_frame_segment({1, 1, 2, 1});
    writer.write_start_of_scan_segment(1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({source.data(), source.size()});

    reader.read_header();

    EXPECT_EQ(size_t{1}, reader.mapping_table_count());
    EXPECT_EQ(0, reader.find_mapping_table_index(1));

    const auto info{reader.get_mapping_table_info(0)};
    EXPECT_EQ(int32_t{1}, info.table_id);
    EXPECT_EQ(int32_t{1}, info.entry_size);
    EXPECT_EQ(uint32_t{100000}, info.data_size);

    vector<byte> table_data(table_size);
    reader.get_mapping_table_data(0, {table_data.data(), table_data.size()});
    EXPECT_EQ(byte{7}, table_data[0]);
    EXPECT_EQ(byte{8}, table_data[table_size - 1]);
}

TEST(jpeg_stream_reader_test, read_mapping_table_continuation_without_mapping_table_throws)
{
    const vector<byte> table_data(255);
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_jpegls_preset_parameters_segment(1, 1, table_data, true);
    writer.write_start_of_frame_segment(1, 1, 8, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_mapping_table_continuation,
                            [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_invalid_mapping_table_continuation_throws)
{
    const vector<byte> table_data(255);
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_jpegls_preset_parameters_segment(1, 1, table_data, false);
    writer.write_jpegls_preset_parameters_segment(1, 2, table_data, true);
    writer.write_start_of_frame_segment(1, 1, 8, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_mapping_table_continuation,
                            [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_define_number_of_lines_16_bit)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 0, 2, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);
    writer.write_define_number_of_lines(1, 2);
    writer.write_start_of_scan_segment(1, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    reader.read_header();
    reader.read_next_start_of_scan();

    EXPECT_EQ(1U, reader.frame_info().height);
}

TEST(jpeg_stream_reader_test, read_define_number_of_lines_24_bit)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 0, 2, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);
    writer.write_define_number_of_lines(1, 3);
    writer.write_start_of_scan_segment(1, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    reader.read_header();
    reader.read_next_start_of_scan();

    EXPECT_EQ(1U, reader.frame_info().height);
}

TEST(jpeg_stream_reader_test, read_define_number_of_lines_32_bit)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 0, 2, 3);
    writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);
    writer.write_define_number_of_lines(numeric_limits<uint32_t>::max(), 4);
    writer.write_start_of_scan_segment(1, 1, 0, interleave_mode::none);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    reader.read_header();
    reader.read_next_start_of_scan();

    EXPECT_EQ(numeric_limits<uint32_t>::max(), reader.frame_info().height);
}

TEST(jpeg_stream_reader_test, read_invalid_height_in_define_number_of_lines_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 0, 2, 3);
    writer.write_start_of_scan_segment(0, 3, 0, interleave_mode::sample);
    writer.write_define_number_of_lines(0, 2);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_parameter_height, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_define_number_of_lines_is_missing_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 0, 2, 3);
    writer.write_start_of_scan_segment(0, 3, 0, interleave_mode::sample);
    writer.write_marker(jpeg_marker_code::end_of_image);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::define_number_of_lines_marker_not_found, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_define_number_of_lines_before_scan_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 0, 2, 3);
    writer.write_define_number_of_lines(1, 2);
    writer.write_start_of_scan_segment(0, 3, 0, interleave_mode::sample);
    writer.write_marker(jpeg_marker_code::end_of_image);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::unexpected_define_number_of_lines_marker, [&reader] { reader.read_header(); });
}

TEST(jpeg_stream_reader_test, read_define_number_of_lines_twice_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 0, 2, 3);
    writer.write_start_of_scan_segment(0, 3, 0, interleave_mode::sample);
    writer.write_define_number_of_lines(1, 2);
    writer.write_define_number_of_lines(1, 2);
    writer.write_marker(jpeg_marker_code::end_of_image);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    reader.read_header();

    assert_expect_exception(jpegls_errc::unexpected_define_number_of_lines_marker,
                            [&reader] { reader.read_next_start_of_scan(); });
}

TEST(jpeg_stream_reader_test, read_define_number_of_lines_invalid_size_throws)
{
    jpeg_test_stream_writer writer;
    writer.write_start_of_image();
    writer.write_start_of_frame_segment(1, 0, 2, 3);
    writer.write_start_of_scan_segment(0, 3, 0, interleave_mode::sample);
    writer.write_define_number_of_lines(1, 5);
    writer.write_marker(jpeg_marker_code::end_of_image);

    jpeg_stream_reader reader;
    reader.source({writer.buffer.data(), writer.buffer.size()});

    assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
}

} // namespace charls::test
