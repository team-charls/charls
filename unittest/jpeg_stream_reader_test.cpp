// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "jpeg_test_stream_writer.h"
#include "util.h"

#include "../src/jpeg_stream_reader.h"
#include "../src/jpeg_stream_writer.h"

#include <array>
#include <cstdint>
#include <vector>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;
using std::numeric_limits;
using std::system_error;
using std::vector;

namespace charls { namespace test {

TEST_CLASS(jpeg_stream_reader_test)
{
public:
    TEST_METHOD(read_header_from_to_small_input_buffer) // NOLINT
    {
        array<uint8_t, 1> buffer{};
        jpeg_stream_reader reader;
        reader.source({buffer.data(), 0});

        assert_expect_exception(jpegls_errc::source_buffer_too_small, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_from_buffer_preceded_with_fill_bytes) // NOLINT
    {
        constexpr uint8_t extra_start_byte{0xFF};
        jpeg_test_stream_writer writer;

        writer.write_byte(extra_start_byte);
        writer.write_start_of_image();

        writer.write_byte(extra_start_byte);
        writer.write_start_of_frame_segment(1, 1, 2, 1);

        writer.write_byte(extra_start_byte);
        writer.write_start_of_scan_segment(0, 1, 128, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        reader.read_header(); // if it doesn't throw test is passed.
    }

    TEST_METHOD(read_header_from_buffer_not_starting_with_ff_should_throw) // NOLINT
    {
        array<uint8_t, 6> buffer{0x0F, 0xFF, 0xD8, 0xFF, 0xFF, 0xDA}; // 0xDA = SOS: Marks the start of scan.

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::jpeg_marker_start_byte_not_found, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_application_data) // NOLINT
    {
        for (uint8_t i{}; i != 16; ++i)
        {
            read_header_with_application_data(i);
        }
    }

    TEST_METHOD(read_header_with_jpegls_extended_frame_should_throw) // NOLINT
    {
        array<uint8_t, 6> buffer{
            0xFF, 0xD8, 0xFF, 0xF9}; // 0xF9 = SOF_57: Marks the start of a JPEG-LS extended (ISO/IEC 14495-2) encoded frame.

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::encoding_not_supported, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_jpegls_preset_parameter_segment) // NOLINT
    {
        vector<uint8_t> source(100);
        jpeg_stream_writer writer({source.data(), source.size()});
        writer.write_start_of_image();

        constexpr jpegls_pc_parameters presets{1, 2, 3, 4, 5};
        writer.write_jpegls_preset_parameters_segment(presets);
        writer.write_start_of_frame_segment({1, 1, 2, 1});
        writer.write_start_of_scan_segment(1, 0, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({source.data(), source.size()});

        reader.read_header();
        const auto& actual = reader.preset_coding_parameters();

        Assert::AreEqual(presets.maximum_sample_value, actual.maximum_sample_value);
        Assert::AreEqual(presets.reset_value, actual.reset_value);
        Assert::AreEqual(presets.threshold1, actual.threshold1);
        Assert::AreEqual(presets.threshold2, actual.threshold2);
        Assert::AreEqual(presets.threshold3, actual.threshold3);
    }

    TEST_METHOD(read_header_with_too_small_jpegls_preset_parameter_segment_should_throw) // NOLINT
    {
        array<uint8_t, 7> buffer{0xFF, 0xD8, 0xFF,
                                 0xF8, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                                 0x00, 0x02, 0x01};

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_small_jpegls_preset_parameter_segment_with_coding_parameters_should_throw) // NOLINT
    {
        array<uint8_t, 7> buffer{0xFF, 0xD8, 0xFF,
                                 0xF8, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                                 0x00, 0x0A, 0x01};

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_large_jpegls_preset_parameter_segment_with_coding_parameters_should_throw) // NOLINT
    {
        array<uint8_t, 7> buffer{0xFF, 0xD8, 0xFF,
                                 0xF8, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                                 0x00, 0x0C, 0x01};

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_jpegls_preset_parameter_with_extended_id_should_throw) // NOLINT
    {
        constexpr array<uint8_t, 8> ids{0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xC, 0xD};

        for (const auto id : ids)
        {
            read_header_with_jpeg_ls_preset_parameter_with_extended_id_should_throw(id);
        }
    }

    TEST_METHOD(read_header_with_too_small_segment_size_should_throw) // NOLINT
    {
        array<uint8_t, 8> buffer{0xFF, 0xD8, 0xFF,
                                 0xF7,                    // SOF_55: Marks the start of JPEG-LS extended scan.
                                 0x00, 0x01, 0xFF, 0xDA}; // SOS: Marks the start of scan.

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_small_start_of_frame_should_throw) // NOLINT
    {
        array<uint8_t, 6> buffer{0xFF, 0xD8, 0xFF,
                                 0xF7, // SOF_55: Marks the start of JPEG-LS extended scan.
                                 0x00, 0x07};

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_small_start_of_frame_in_component_info_should_throw) // NOLINT
    {
        array<uint8_t, 6> buffer{0xFF, 0xD8, 0xFF,
                                 0xF7, // SOF_55: Marks the start of JPEG-LS extended scan.
                                 0x00, 0x07};

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_large_start_of_frame_should_throw) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.buffer.push_back(0);
        writer.buffer[5]++;

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_sos_before_sof_should_throw) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_scan_segment(0, 1, 128, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::unexpected_marker_found, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_extra_sof_should_throw) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_start_of_frame_segment(512, 512, 8, 3);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::duplicate_start_of_frame_marker, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_too_large_near_lossless_in_sos_should_throw) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 128, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});
        reader.read_header();

        assert_expect_exception(jpegls_errc::invalid_parameter_near_lossless, [&reader] { reader.read_start_of_scan(); });
    }

    TEST_METHOD(read_header_too_large_near_lossless_in_sos_should_throw2) // NOLINT
    {
        constexpr jpegls_pc_parameters preset_coding_parameters{200, 0, 0, 0, 0};

        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_jpegls_preset_parameters_segment(preset_coding_parameters);
        writer.write_start_of_frame_segment(512, 512, 8, 3);

        constexpr int bad_near_lossless = (200 / 2) + 1;
        writer.write_start_of_scan_segment(0, 1, bad_near_lossless, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});
        reader.read_header();

        assert_expect_exception(jpegls_errc::invalid_parameter_near_lossless, [&reader] { reader.read_start_of_scan(); });
    }

    TEST_METHOD(read_header_line_interleave_in_sos_for_single_component_should_throw) // NOLINT
    {
        read_header_incorrect_interleave_in_sos_for_single_component_should_throw(interleave_mode::line);
    }

    TEST_METHOD(read_header_sample_interleave_in_sos_for_single_component_should_throw) // NOLINT
    {
        read_header_incorrect_interleave_in_sos_for_single_component_should_throw(interleave_mode::sample);
    }

    TEST_METHOD(read_header_with_duplicate_component_id_in_start_of_frame_segment_should_throw) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.componentIdOverride = 7;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::duplicate_component_id_in_sof_segment, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_small_start_of_scan_should_throw) // NOLINT
    {
        array<uint8_t, 16> buffer{0xFF, 0xD8, 0xFF,
                                  0xF7, // SOF_55: Marks the start of JPEG-LS extended scan.
                                  0x00,
                                  0x08, // size
                                  0x08, // bits per sample
                                  0x00,
                                  0x01, // width
                                  0x00,
                                  0x01, // height
                                  0x01, // component count
                                  0xFF,
                                  0xDA, // SOS
                                  0x00, 0x03};

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_small_start_of_scan_component_count_should_throw) // NOLINT
    {
        array<uint8_t, 17> buffer{0xFF, 0xD8, 0xFF,
                                  0xF7, // SOF_55: Marks the start of JPEG-LS extended scan.
                                  0x00,
                                  0x08, // size
                                  0x08, // bits per sample
                                  0x00,
                                  0x01, // width
                                  0x00,
                                  0x01, // height
                                  0x01, // component count
                                  0xFF,
                                  0xDA, // SOS
                                  0x00, 0x07, 0x01};

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_directly_end_of_image_should_throw) // NOLINT
    {
        array<uint8_t, 4> buffer{0xFF, 0xD8, 0xFF, 0xD9}; // 0xD9 = EOI

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::unexpected_end_of_image_marker, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_duplicate_start_of_image_should_throw) // NOLINT
    {
        array<uint8_t, 4> buffer{0xFF, 0xD8, 0xFF, 0xD8}; // 0xD8 = SOI.

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::duplicate_start_of_image_marker, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_spiff_header) // NOLINT
    {
        read_spiff_header(0);
    }

    TEST_METHOD(read_spiff_header_low_version_newer) // NOLINT
    {
        read_spiff_header(1);
    }

    TEST_METHOD(read_spiff_header_high_version_to_new) // NOLINT
    {
        vector<uint8_t> buffer{create_test_spiff_header(3)};
        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        spiff_header spiff_header{};
        bool spiff_header_found{};

        reader.read_header(&spiff_header, &spiff_header_found);

        Assert::IsFalse(spiff_header_found);
    }

    TEST_METHOD(read_spiff_header_without_end_of_directory) // NOLINT
    {
        vector<uint8_t> buffer = create_test_spiff_header(2, 0, false);
        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        spiff_header spiff_header{};
        bool spiff_header_found{};

        reader.read_header(&spiff_header, &spiff_header_found);
        Assert::IsTrue(spiff_header_found);

        assert_expect_exception(jpegls_errc::missing_end_of_spiff_directory, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_define_restart_interval_16_bit) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_define_restart_interval(numeric_limits<uint16_t>::max() - 5, 2);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});
        reader.read_header();

        Assert::AreEqual(static_cast<uint32_t>(numeric_limits<uint16_t>::max() - 5), reader.parameters().restart_interval);
    }

    TEST_METHOD(read_header_with_define_restart_interval_24_bit) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_define_restart_interval(numeric_limits<uint16_t>::max() + 5, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});
        reader.read_header();

        Assert::AreEqual(static_cast<uint32_t>(numeric_limits<uint16_t>::max() + 5), reader.parameters().restart_interval);
    }

    TEST_METHOD(read_header_with_define_restart_interval_32_bit) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_define_restart_interval(numeric_limits<uint32_t>::max() - 7, 4);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});
        reader.read_header();

        Assert::AreEqual(numeric_limits<uint32_t>::max() - 7, reader.parameters().restart_interval);
    }

    TEST_METHOD(read_header_with_2_define_restart_intervals) // NOLINT
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

        Assert::AreEqual(0U, reader.parameters().restart_interval);
    }

    TEST_METHOD(read_header_with_bad_define_restart_interval) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);

        constexpr array<uint8_t, 1> buffer{};
        writer.write_segment(jpeg_marker_code::define_restart_interval, buffer.data(), buffer.size());
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_jpegls_stream_with_restart_marker_outside_entropy_data) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_restart_marker(0);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::unexpected_restart_marker, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_comment) // NOLINT
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

        reader.at_comment(
            [](const void* data, const size_t size, void* user_context) noexcept -> int32_t {
                auto* actual_output = static_cast<callback_output*>(user_context);
                actual_output->data = data;
                actual_output->size = size;
                return 0;
            },
            &actual);

        reader.read_header();

        Assert::AreEqual(static_cast<size_t>(5), actual.size);
        Assert::IsTrue(memcmp("hello", actual.data, actual.size) == 0);
    }

    TEST_METHOD(read_empty_comment) // NOLINT
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

        reader.at_comment(
            [](const void* data, const size_t size, void* user_context) noexcept -> int32_t {
                auto* actual_output = static_cast<callback_output*>(user_context);
                actual_output->data = data;
                actual_output->size = size;
                return 0;
            },
            &actual);

        reader.read_header();

        Assert::AreEqual(static_cast<size_t>(0), actual.size);
        Assert::IsNull(actual.data);
    }

    TEST_METHOD(read_comment_from_too_small_buffer_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_segment(jpeg_marker_code::comment, "", 0);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size() - 1});

        bool called{};
        reader.at_comment(
            [](const void*, const size_t, void* user_context) noexcept -> int32_t {
                auto* actual_called = static_cast<bool*>(user_context);
                *actual_called = true;
                return 0;
            },
            &called);

        assert_expect_exception(jpegls_errc::source_buffer_too_small, [&reader] { reader.read_header(); });
        Assert::IsFalse(called);
    }


private:
    static void read_spiff_header(const uint8_t low_version)
    {
        vector<uint8_t> buffer = create_test_spiff_header(2, low_version);
        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        spiff_header spiff_header{};
        bool spiff_header_found{};

        reader.read_header(&spiff_header, &spiff_header_found);

        Assert::IsTrue(spiff_header_found);
        Assert::AreEqual(static_cast<int32_t>(spiff_profile_id::none), static_cast<int32_t>(spiff_header.profile_id));
        Assert::AreEqual(3, spiff_header.component_count);
        Assert::AreEqual(800U, spiff_header.height);
        Assert::AreEqual(600U, spiff_header.width);
        Assert::AreEqual(static_cast<int32_t>(spiff_color_space::rgb), static_cast<int32_t>(spiff_header.color_space));
        Assert::AreEqual(8, spiff_header.bits_per_sample);
        Assert::AreEqual(static_cast<int32_t>(spiff_compression_type::jpeg_ls),
                         static_cast<int32_t>(spiff_header.compression_type));
        Assert::AreEqual(static_cast<int32_t>(spiff_resolution_units::dots_per_inch),
                         static_cast<int32_t>(spiff_header.resolution_units));
        Assert::AreEqual(96U, spiff_header.vertical_resolution);
        Assert::AreEqual(1024U, spiff_header.horizontal_resolution);
    }

    static void read_header_with_application_data(const uint8_t data_number)
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();

        writer.write_byte(0xFF);
        writer.write_byte(0xE0 + data_number);
        writer.write_byte(0x00);
        writer.write_byte(0x02);

        writer.write_start_of_frame_segment(1, 1, 2, 1);
        writer.write_start_of_scan_segment(0, 1, 128, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        reader.read_header(); // if it doesn't throw test is passed.
    }

    static void read_header_incorrect_interleave_in_sos_for_single_component_should_throw(const interleave_mode mode)
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 1);
        writer.write_start_of_scan_segment(0, 1, 0, mode);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});
        reader.read_header();

        assert_expect_exception(jpegls_errc::invalid_parameter_interleave_mode, [&reader] { reader.read_start_of_scan(); });
    }

    static void read_header_with_jpeg_ls_preset_parameter_with_extended_id_should_throw(const uint8_t id)
    {
        array<uint8_t, 7> buffer{0xFF, 0xD8, 0xFF,
                                 0xF8, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                                 0x00, 0x03, id};

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::jpegls_preset_extended_parameter_type_not_supported,
                                [&reader] { reader.read_header(); });
    }
};

}} // namespace charls::test
