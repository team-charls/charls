// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "jpeg_test_stream_writer.hpp"
#include "util.hpp"

#include "../src/jpeg_stream_reader.hpp"
#include "../src/jpeg_stream_writer.hpp"

#include <array>
#include <cstdint>
#include <vector>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;
using std::byte;
using std::numeric_limits;
using std::vector;

namespace charls::test {

TEST_CLASS(jpeg_stream_reader_test)
{
public:
    TEST_METHOD(read_header_from_to_small_input_buffer_throws) // NOLINT
    {
        array<byte, 1> buffer{};
        jpeg_stream_reader reader;
        reader.source({buffer.data(), 0});

        assert_expect_exception(jpegls_errc::need_more_data, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_from_buffer_preceded_with_fill_bytes) // NOLINT
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

    TEST_METHOD(read_header_from_buffer_not_starting_with_ff_throws) // NOLINT
    {
        constexpr array buffer{byte{0x0F}, byte{0xFF}, byte{0xD8},
                               byte{0xFF}, byte{0xFF}, byte{0xDA}}; // 0xDA = SOS: Marks the start of scan.

        jpeg_stream_reader reader;
        reader.source(buffer);

        assert_expect_exception(jpegls_errc::jpeg_marker_start_byte_not_found, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_application_data) // NOLINT
    {
        for (uint8_t i{}; i != 16; ++i)
        {
            read_header_with_application_data(i);
        }
    }

    TEST_METHOD(read_header_with_jpegls_extended_frame_throws) // NOLINT
    {
        constexpr array<byte, 6> buffer{
            byte{0xFF}, byte{0xD8}, byte{0xFF},
            byte{0xF9}}; // 0xF9 = SOF_57: Marks the start of a JPEG-LS extended (ISO/IEC 14495-2) encoded frame.

        jpeg_stream_reader reader;
        reader.source(buffer);

        assert_expect_exception(jpegls_errc::encoding_not_supported, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_jpegls_preset_parameter_segment) // NOLINT
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

        Assert::AreEqual(presets.maximum_sample_value, maximum_sample_value);
        Assert::AreEqual(presets.reset_value, reset_value);
        Assert::AreEqual(presets.threshold1, threshold1);
        Assert::AreEqual(presets.threshold2, threshold2);
        Assert::AreEqual(presets.threshold3, threshold3);
    }

    TEST_METHOD(read_header_with_too_small_jpegls_preset_parameter_segment_throws) // NOLINT
    {
        constexpr array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF},
                               byte{0xF8}, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                               byte{0x00}, byte{0x02}, byte{0x01}};

        jpeg_stream_reader reader;
        reader.source(buffer);

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_small_jpegls_preset_parameter_segment_with_coding_parameters_throws) // NOLINT
    {
        constexpr array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF},
                               byte{0xF8}, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                               byte{0x00}, byte{0x0A}, byte{0x01}};

        jpeg_stream_reader reader;
        reader.source(buffer);

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_large_jpegls_preset_parameter_segment_with_coding_parameters_throws) // NOLINT
    {
        constexpr array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF},
                               byte{0xF8}, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                               byte{0x00}, byte{0x0C}, byte{0x01}};

        jpeg_stream_reader reader;
        reader.source(buffer);

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_jpegls_preset_parameter_with_extended_id_throws) // NOLINT
    {
        constexpr array ids{byte{0x5}, byte{0x6}, byte{0x7}, byte{0x8}, byte{0x9}, byte{0xA}, byte{0xC}, byte{0xD}};

        for (const auto id : ids)
        {
            read_header_with_jpeg_ls_preset_parameter_with_extended_id_throws(id);
        }
    }

    TEST_METHOD(read_header_with_too_small_segment_size_throws) // NOLINT
    {
        constexpr array buffer{
            byte{0xFF}, byte{0xD8}, byte{0xFF}, byte{0xF7},  // SOF_55: Marks the start of JPEG-LS extended scan.
            byte{0x00}, byte{0x01}, byte{0xFF}, byte{0xDA}}; // SOS: Marks the start of scan.

        jpeg_stream_reader reader;
        reader.source(buffer);

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_small_start_of_frame_throws) // NOLINT
    {
        constexpr array buffer{
            byte{0xFF}, byte{0xD8}, byte{0xFF}, byte{0xF7}, // SOF_55: Marks the start of JPEG-LS extended scan.
            byte{0x00}, byte{0x06}, byte{2},    byte{2},    byte{2}, byte{2}, byte{2}, byte{2}, byte{1}};

        jpeg_stream_reader reader;
        reader.source(buffer);

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_small_start_of_frame_in_component_info_throws) // NOLINT
    {
        constexpr array buffer{
            byte{0xFF}, byte{0xD8}, byte{0xFF}, byte{0xF7}, // SOF_55: Marks the start of JPEG-LS extended scan.
            byte{0x00}, byte{0x08}, byte{2},    byte{2},    byte{2}, byte{2}, byte{2}, byte{2}, byte{1}};

        jpeg_stream_reader reader;
        reader.source(buffer);

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_large_start_of_frame_throws) // NOLINT
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

    TEST_METHOD(read_header_sos_before_sof_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_scan_segment(0, 1, 128, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::unexpected_start_of_scan_marker, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_extra_sof_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_start_of_frame_segment(512, 512, 8, 3);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::duplicate_start_of_frame_marker, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_too_large_near_lossless_in_sos_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 128, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_parameter_near_lossless, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_too_large_near_lossless_in_sos_throws2) // NOLINT
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

    TEST_METHOD(read_header_line_interleave_in_sos_for_single_component_throws) // NOLINT
    {
        read_header_incorrect_interleave_in_sos_for_single_component_throws(interleave_mode::line);
    }

    TEST_METHOD(read_header_sample_interleave_in_sos_for_single_component_throws) // NOLINT
    {
        read_header_incorrect_interleave_in_sos_for_single_component_throws(interleave_mode::sample);
    }

    TEST_METHOD(read_header_with_duplicate_component_id_in_start_of_frame_segment_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.componentIdOverride = 7;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::duplicate_component_id_in_sof_segment, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_to_many_components_in_start_of_frame_segment_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 1);
        writer.write_start_of_scan_segment(0, 2, 0, interleave_mode::sample);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_parameter_component_count, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_no_components_in_start_of_frame_segment_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 1);
        writer.write_start_of_scan_segment(0, 0, 0, interleave_mode::none);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_parameter_component_count, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_more_then_max_components_in_start_of_frame_segment_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 5);
        writer.write_start_of_scan_segment(0, 5, 0, interleave_mode::sample);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_parameter_component_count, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_too_small_start_of_scan_throws) // NOLINT
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

    TEST_METHOD(read_header_with_too_small_start_of_scan_component_count_throws) // NOLINT
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

    TEST_METHOD(read_header_with_directly_end_of_image_throws) // NOLINT
    {
        constexpr array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF}, byte{0xD9}}; // 0xD9 = EOI

        jpeg_stream_reader reader;
        reader.source(buffer);

        assert_expect_exception(jpegls_errc::unexpected_end_of_image_marker, [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_header_with_duplicate_start_of_image_throws) // NOLINT
    {
        constexpr array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF}, byte{0xD8}}; // 0xD8 = SOI.

        jpeg_stream_reader reader;
        reader.source(buffer);

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
        const auto buffer{create_test_spiff_header(3)};
        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        spiff_header spiff_header{};
        bool spiff_header_found{};

        reader.read_header(&spiff_header, &spiff_header_found);

        Assert::IsFalse(spiff_header_found);
    }

    TEST_METHOD(read_spiff_header_without_end_of_directory) // NOLINT
    {
        const auto buffer{create_test_spiff_header(2, 0, false)};
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
        writer.write_define_restart_interval(numeric_limits<uint16_t>::max() - 5U, 2);
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
        writer.write_define_restart_interval(numeric_limits<uint16_t>::max() + 5U, 3);
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

        constexpr array<byte, 1> buffer{};
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

        reader.at_comment({[](const void* data, const size_t size, void* user_context) noexcept -> int32_t {
                               auto* actual_output{static_cast<callback_output*>(user_context)};
                               actual_output->data = data;
                               actual_output->size = size;
                               return 0;
                           },
                           &actual});

        reader.read_header();

        Assert::AreEqual(size_t{5}, actual.size);
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

        reader.at_comment({[](const void* data, const size_t size, void* user_context) noexcept -> int32_t {
                               auto* actual_output{static_cast<callback_output*>(user_context)};
                               actual_output->data = data;
                               actual_output->size = size;
                               return 0;
                           },
                           &actual});

        reader.read_header();

        Assert::AreEqual(size_t{}, actual.size);
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
        reader.at_comment({[](const void*, const size_t, void* user_context) noexcept -> int32_t {
                               auto* actual_called{static_cast<bool*>(user_context)};
                               *actual_called = true;
                               return 0;
                           },
                           &called});

        assert_expect_exception(jpegls_errc::need_more_data, [&reader] { reader.read_header(); });
        Assert::IsFalse(called);
    }

    TEST_METHOD(read_application_data) // NOLINT
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

        Assert::AreEqual(8, actual.id);
        Assert::AreEqual(size_t{5}, actual.size);
        Assert::IsTrue(memcmp("hello", actual.data, actual.size) == 0);
    }

    TEST_METHOD(read_empty_application_data) // NOLINT
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

        Assert::AreEqual(15, actual.id);
        Assert::AreEqual(size_t{}, actual.size);
        Assert::IsNull(actual.data);
    }

    TEST_METHOD(read_application_data_from_too_small_buffer_throws) // NOLINT
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
        Assert::IsFalse(called);
    }

    TEST_METHOD(read_mapping_table) // NOLINT
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

        Assert::AreEqual(size_t{1}, reader.mapping_table_count());
        Assert::AreEqual(0, reader.find_mapping_table_index(1));

        const auto info{reader.get_mapping_table_info(0)};
        Assert::AreEqual(int32_t{1}, info.table_id);
        Assert::AreEqual(int32_t{1}, info.entry_size);
        Assert::AreEqual(uint32_t{1}, info.data_size);

        vector<byte> table_data(1);
        reader.get_mapping_table_data(0, {table_data.data(), table_data.size()});
        Assert::AreEqual(byte{2}, table_data[0]);
    }

    TEST_METHOD(read_mapping_table_too_small_buffer_throws) // NOLINT
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

    TEST_METHOD(mapping_table_count_is_zero_at_start) // NOLINT
    {
        const jpeg_stream_reader reader;

        const auto count{reader.mapping_table_count()};

        Assert::AreEqual(size_t{}, count);
    }

    TEST_METHOD(mapping_table_count_after_read_header) // NOLINT
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

        Assert::AreEqual(size_t{2}, count);
    }

    TEST_METHOD(mapping_table_count_after_read_header_after_frame) // NOLINT
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

        Assert::AreEqual(size_t{1}, count);
    }

    TEST_METHOD(mapping_table_count_after_read_header_before_frame) // NOLINT
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

        Assert::AreEqual(size_t{1}, count);
    }

    TEST_METHOD(read_mapping_table_continuation) // NOLINT
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

        Assert::AreEqual(size_t{1}, reader.mapping_table_count());
        Assert::AreEqual(0, reader.find_mapping_table_index(1));

        const auto info{reader.get_mapping_table_info(0)};
        Assert::AreEqual(int32_t{1}, info.table_id);
        Assert::AreEqual(int32_t{1}, info.entry_size);
        Assert::AreEqual(uint32_t{100000}, info.data_size);

        vector<byte> table_data(table_size);
        reader.get_mapping_table_data(0, {table_data.data(), table_data.size()});
        Assert::AreEqual(byte{7}, table_data[0]);
        Assert::AreEqual(byte{8}, table_data[table_size - 1]);
    }

    TEST_METHOD(read_mapping_table_continuation_without_mapping_table_throws) // NOLINT
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

    TEST_METHOD(read_invalid_mapping_table_continuation_throws) // NOLINT
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

    TEST_METHOD(read_define_number_of_lines_16_bit)
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

        Assert::AreEqual(1U, reader.frame_info().height);
    }

    TEST_METHOD(read_define_number_of_lines_24_bit)
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

        Assert::AreEqual(1U, reader.frame_info().height);
    }

    TEST_METHOD(read_define_number_of_lines_32_bit)
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

        Assert::AreEqual(numeric_limits<uint32_t>::max(), reader.frame_info().height);
    }

    TEST_METHOD(read_invalid_height_in_define_number_of_lines_throws)
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(1, 0, 2, 3);
        writer.write_start_of_scan_segment(0, 3, 0, interleave_mode::sample);
        writer.write_define_number_of_lines(0, 2);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_parameter_height,
                                [&reader] { reader.read_header(); });
    }

    TEST_METHOD(read_define_number_of_lines_is_missing_throws)
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

    TEST_METHOD(read_define_number_of_lines_before_scan_throws)
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

    TEST_METHOD(read_define_number_of_lines_twice_throws)
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

    TEST_METHOD(read_define_number_of_lines_invalid_size_throws)
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

private:
    static void read_spiff_header(const uint8_t low_version)
    {
        auto buffer{create_test_spiff_header(2, low_version)};
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

    static void read_header_incorrect_interleave_in_sos_for_single_component_throws(const interleave_mode mode)
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 1);
        writer.write_start_of_scan_segment(0, 1, 0, mode);

        jpeg_stream_reader reader;
        reader.source({writer.buffer.data(), writer.buffer.size()});

        assert_expect_exception(jpegls_errc::invalid_parameter_interleave_mode, [&reader] { reader.read_header(); });
    }

    static void read_header_with_jpeg_ls_preset_parameter_with_extended_id_throws(const byte id)
    {
        const array buffer{byte{0xFF}, byte{0xD8}, byte{0xFF},
                           byte{0xF8}, // LSE: Marks the start of a JPEG-LS preset parameters segment.
                           byte{0x00}, byte{0x03}, id};

        jpeg_stream_reader reader;
        reader.source({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::jpegls_preset_extended_parameter_type_not_supported,
                                [&reader] { reader.read_header(); });
    }
};

} // namespace charls::test
