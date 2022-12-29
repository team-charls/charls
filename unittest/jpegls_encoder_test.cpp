// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "jpegls_preset_coding_parameters_test.h"
#include "util.h"

#include "../src/jpeg_marker_code.h"
#include <charls/charls.h>

#include <array>
#include <limits>
#include <tuple>
#include <vector>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;
using std::ignore;
using std::numeric_limits;
using std::vector;
using namespace std::string_literals;

constexpr size_t serialized_spiff_header_size{34};

namespace charls { namespace test {

TEST_CLASS(jpegls_encoder_test)
{
public:
    TEST_METHOD(create_destroy) // NOLINT
    {
        // ReSharper disable once CppLocalVariableWithNonTrivialDtorIsNeverUsed
        jpegls_encoder encoder;
    }

    TEST_METHOD(create_and_move) // NOLINT
    {
        jpegls_encoder encoder1;

        jpegls_encoder encoder2(std::move(encoder1));

        jpegls_encoder encoder3;
        array<uint8_t, 10> buffer{};
        encoder3.destination(buffer.data(), buffer.size());
        encoder3 = std::move(encoder2);
    }

    TEST_METHOD(frame_info_max_and_min) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});                                                                     // minimum.
        encoder.frame_info({std::numeric_limits<uint32_t>::max(), numeric_limits<uint32_t>::max(), 16, 255}); // maximum.
    }

    TEST_METHOD(frame_info_bad_width_throws) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_width, [&encoder] { encoder.frame_info({0, 1, 2, 1}); });
    }

    TEST_METHOD(frame_info_bad_height_throws) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_height, [&encoder] { encoder.frame_info({1, 0, 2, 1}); });
    }

    TEST_METHOD(frame_info_bad_bits_per_sample_throws) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_bits_per_sample, [&encoder] {
            encoder.frame_info({1, 1, 1, 1});
        });
        assert_expect_exception(jpegls_errc::invalid_argument_bits_per_sample, [&encoder] {
            encoder.frame_info({1, 1, 17, 1});
        });
    }

    TEST_METHOD(frame_info_bad_component_count_throws) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_component_count, [&encoder] {
            encoder.frame_info({1, 1, 2, 0});
        });
        assert_expect_exception(jpegls_errc::invalid_argument_component_count, [&encoder] {
            encoder.frame_info({1, 1, 2, 256});
        });
    }

    TEST_METHOD(interleave_mode) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.interleave_mode(interleave_mode::none);
        encoder.interleave_mode(interleave_mode::line);
        encoder.interleave_mode(interleave_mode::sample);
    }

    TEST_METHOD(interleave_mode_bad_throws) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_interleave_mode,
                                [&encoder] { encoder.interleave_mode(static_cast<charls::interleave_mode>(-1)); });
        assert_expect_exception(jpegls_errc::invalid_argument_interleave_mode,
                                [&encoder] { encoder.interleave_mode(static_cast<charls::interleave_mode>(3)); });
    }

    TEST_METHOD(interleave_mode_does_not_match_component_count_throws) // NOLINT
    {
        constexpr frame_info frame_info{512, 512, 8, 1};
        vector<uint8_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        assert_expect_exception(jpegls_errc::invalid_argument_interleave_mode, [&frame_info, &source] {
            jpegls_encoder::encode(source, frame_info, interleave_mode::sample);
        });
        assert_expect_exception(jpegls_errc::invalid_argument_interleave_mode, [&frame_info, &source] {
            jpegls_encoder::encode(source, frame_info, interleave_mode::line);
        });
    }

    TEST_METHOD(near_lossless) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.near_lossless(0);   // set lowest value.
        encoder.near_lossless(255); // set highest value.
    }

    TEST_METHOD(near_lossless_bad_throws) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_near_lossless, [&encoder] { encoder.near_lossless(-1); });
        assert_expect_exception(jpegls_errc::invalid_argument_near_lossless, [&encoder] { encoder.near_lossless(256); });
    }

    TEST_METHOD(estimated_destination_size_minimal_frame_info) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1}); // = minimum.
        const auto size{encoder.estimated_destination_size()};
        Assert::IsTrue(size >= 1024);
    }

    TEST_METHOD(estimated_destination_size_maximal_frame_info) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({numeric_limits<uint16_t>::max(), numeric_limits<uint16_t>::max(), 8, 1}); // = maximum.
        const auto size{encoder.estimated_destination_size()};
        constexpr auto expected{static_cast<size_t>(numeric_limits<uint16_t>::max()) * numeric_limits<uint16_t>::max() * 1 *
                                1};
        Assert::IsTrue(size >= expected);
    }

    TEST_METHOD(estimated_destination_size_monochrome_16_bit) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({100, 100, 16, 1}); // minimum.
        const auto size{encoder.estimated_destination_size()};
        Assert::IsTrue(size >= size_t{100} * 100 * 2);
    }

    TEST_METHOD(estimated_destination_size_color_8_bit) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({2000, 2000, 8, 3});
        const auto size{encoder.estimated_destination_size()};
        Assert::IsTrue(size >= size_t{2000} * 2000 * 3);
    }

    TEST_METHOD(estimated_destination_size_very_wide) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({numeric_limits<uint16_t>::max(), 1, 8, 1});
        const auto size{encoder.estimated_destination_size()};
        Assert::IsTrue(size >= static_cast<size_t>(numeric_limits<uint16_t>::max()) + 1024U);
    }

    TEST_METHOD(estimated_destination_size_very_high) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, numeric_limits<uint16_t>::max(), 8, 1});
        const auto size{encoder.estimated_destination_size()};
        Assert::IsTrue(size >= static_cast<size_t>(numeric_limits<uint16_t>::max()) + 1024U);
    }

    TEST_METHOD(estimated_destination_size_too_soon_throws) // NOLINT
    {
        const jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&encoder] { ignore = encoder.estimated_destination_size(); });
    }

    TEST_METHOD(estimated_destination_size_thath_causes_overflow_throws) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({numeric_limits<uint32_t>::max(), numeric_limits<uint32_t>::max(), 8, 1});

#if INTPTR_MAX == INT64_MAX
        const auto size{encoder.estimated_destination_size()};
        Assert::IsTrue(size != 0); // actual value already checked in other test functions.
#elif INTPTR_MAX == INT32_MAX
        assert_expect_exception(jpegls_errc::parameter_value_not_supported,
                                [&encoder] { ignore = encoder.estimated_destination_size(); });
#else
#error Unknown pointer size or missing size macros!
#endif
    }

    TEST_METHOD(destination) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(200);
        encoder.destination(destination);
    }

    TEST_METHOD(destination_can_only_be_set_once_throws) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(200);
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&encoder, &destination] { encoder.destination(destination); });
    }

    TEST_METHOD(write_standard_spiff_header) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        Assert::AreEqual(serialized_spiff_header_size + 2, encoder.bytes_written());

        // Check that SOI marker has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that a APP8 with SPIFF has been written (details already verified by jpeg_stream_writer_test).
        Assert::AreEqual(uint8_t{0xFF}, destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::application_data8), destination[3]);
        Assert::AreEqual(uint8_t{}, destination[4]);
        Assert::AreEqual(uint8_t{32}, destination[5]);
        Assert::AreEqual(uint8_t{'S'}, destination[6]);
        Assert::AreEqual(uint8_t{'P'}, destination[7]);
        Assert::AreEqual(uint8_t{'I'}, destination[8]);
        Assert::AreEqual(uint8_t{'F'}, destination[9]);
        Assert::AreEqual(uint8_t{'F'}, destination[10]);
        Assert::AreEqual(uint8_t{}, destination[11]);
    }

    TEST_METHOD(write_standard_spiff_header_without_destination_throws) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&encoder] { encoder.write_standard_spiff_header(spiff_color_space::cmyk); });
    }

    TEST_METHOD(write_standard_spiff_header_without_frame_info_throws) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(100);
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&encoder] { encoder.write_standard_spiff_header(spiff_color_space::cmyk); });
    }

    TEST_METHOD(write_standard_spiff_header_twice_throws) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&encoder] { encoder.write_standard_spiff_header(spiff_color_space::cmyk); });
    }

    TEST_METHOD(write_spiff_header) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        spiff_header spiff_header{};
        spiff_header.width = 1;
        spiff_header.height = 1;
        encoder.write_spiff_header(spiff_header);

        Assert::AreEqual(serialized_spiff_header_size + 2, encoder.bytes_written());

        // Check that SOI marker has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that a APP8 with SPIFF has been written (details already verified by jpeg_stream_writer_test).
        Assert::AreEqual(uint8_t{0xFF}, destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::application_data8), destination[3]);
        Assert::AreEqual(uint8_t{}, destination[4]);
        Assert::AreEqual(uint8_t{32}, destination[5]);
        Assert::AreEqual(uint8_t{'S'}, destination[6]);
        Assert::AreEqual(uint8_t{'P'}, destination[7]);
        Assert::AreEqual(uint8_t{'I'}, destination[8]);
        Assert::AreEqual(uint8_t{'F'}, destination[9]);
        Assert::AreEqual(uint8_t{'F'}, destination[10]);
        Assert::AreEqual(uint8_t{}, destination[11]);
    }

    TEST_METHOD(write_spiff_header_invalid_height_throws) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        spiff_header spiff_header{};
        spiff_header.width = 1;

        assert_expect_exception(jpegls_errc::invalid_argument_height,
                                [&encoder, &spiff_header] { encoder.write_spiff_header(spiff_header); });
        Assert::AreEqual(size_t{}, encoder.bytes_written());
    }

    TEST_METHOD(write_spiff_header_invalid_width_throws) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        spiff_header spiff_header{};
        spiff_header.height = 1;

        assert_expect_exception(jpegls_errc::invalid_argument_width,
                                [&encoder, &spiff_header] { encoder.write_spiff_header(spiff_header); });
        Assert::AreEqual(size_t{}, encoder.bytes_written());
    }

    TEST_METHOD(write_spiff_entry) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        encoder.write_spiff_entry(spiff_entry_tag::image_title, "test", 4);

        Assert::AreEqual(size_t{48}, encoder.bytes_written());
    }

    TEST_METHOD(write_spiff_entry_twice) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        encoder.write_spiff_entry(spiff_entry_tag::image_title, "test", 4);
        encoder.write_spiff_entry(spiff_entry_tag::image_title, "test", 4);

        Assert::AreEqual(size_t{60}, encoder.bytes_written());
    }

    TEST_METHOD(write_empty_spiff_entry) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        encoder.write_spiff_entry(spiff_entry_tag::image_title, nullptr, 0);

        Assert::AreEqual(size_t{44}, encoder.bytes_written());
    }

    TEST_METHOD(write_spiff_entry_with_invalid_tag_throws) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        assert_expect_exception(jpegls_errc::invalid_argument, [&encoder] { encoder.write_spiff_entry(1, "test", 4); });
    }

    TEST_METHOD(write_spiff_entry_with_invalid_size_throws) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        assert_expect_exception(jpegls_errc::invalid_argument_size, [&encoder] {
            const vector<uint8_t> spiff_entry(65528 + 1);
            encoder.write_spiff_entry(spiff_entry_tag::image_title, spiff_entry.data(), spiff_entry.size());
        });
    }

    TEST_METHOD(write_spiff_entry_without_spiff_header_throws) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_operation, [&encoder] {
            const vector<uint8_t> spiff_entry(65528);
            encoder.write_spiff_entry(spiff_entry_tag::image_title, spiff_entry.data(), spiff_entry.size());
        });
    }

    TEST_METHOD(write_spiff_end_of_directory_entry) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(300);
        encoder.destination(destination);

        encoder.write_standard_spiff_header(spiff_color_space::none);
        encoder.write_spiff_end_of_directory_entry();

        Assert::AreEqual(uint8_t{0xFF}, destination[44]);
        Assert::AreEqual(uint8_t{0xD8}, destination[45]); // 0xD8 = SOI: Marks the start of an image.
    }

    TEST_METHOD(write_spiff_end_of_directory_entry_before_header_throws) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(300);
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&encoder] { encoder.write_spiff_end_of_directory_entry(); });
    }

    TEST_METHOD(write_spiff_end_of_directory_entry_twice_throws) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(300);
        encoder.destination(destination);

        encoder.write_standard_spiff_header(spiff_color_space::none);
        encoder.write_spiff_end_of_directory_entry();

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&encoder] { encoder.write_spiff_end_of_directory_entry(); });
    }

    TEST_METHOD(write_comment) // NOLINT
    {
        jpegls_encoder encoder;

        array<uint8_t, 10> destination;
        encoder.destination(destination);

        encoder.write_comment("123");

        Assert::AreEqual(size_t{10}, encoder.bytes_written());

        // Check that SOI marker has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that a COM segment has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::comment), destination[3]);
        Assert::AreEqual(uint8_t{}, destination[4]);
        Assert::AreEqual(uint8_t{2 + 4}, destination[5]);
        Assert::AreEqual(uint8_t{'1'}, destination[6]);
        Assert::AreEqual(uint8_t{'2'}, destination[7]);
        Assert::AreEqual(uint8_t{'3'}, destination[8]);
        Assert::AreEqual(uint8_t{}, destination[9]);
    }

    TEST_METHOD(write_empty_comment) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(6);
        encoder.destination(destination);

        encoder.write_comment("");

        Assert::AreEqual(size_t{6}, encoder.bytes_written());

        // Check that SOI marker has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that a COM segment has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::comment), destination[3]);
        Assert::AreEqual(uint8_t{}, destination[4]);
        Assert::AreEqual(uint8_t{2}, destination[5]);
    }

    TEST_METHOD(write_empty_comment_buffer) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(6);
        encoder.destination(destination);

        encoder.write_comment(nullptr, 0);

        Assert::AreEqual(size_t{6}, encoder.bytes_written());

        // Check that SOI marker has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that a COM segment has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::comment), destination[3]);
        Assert::AreEqual(uint8_t{}, destination[4]);
        Assert::AreEqual(uint8_t{2}, destination[5]);
    }

    TEST_METHOD(write_max_comment) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(2 + 2 + static_cast<size_t>(numeric_limits<uint16_t>::max()));
        encoder.destination(destination);

        constexpr size_t max_size_comment_data{static_cast<size_t>(numeric_limits<uint16_t>::max()) - 2};
        const vector<uint8_t> data(max_size_comment_data);
        encoder.write_comment(data.data(), data.size());

        Assert::AreEqual(destination.size(), encoder.bytes_written());

        // Check that SOI marker has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that a COM segment has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::comment), destination[3]);
        Assert::AreEqual(uint8_t{255}, destination[4]);
        Assert::AreEqual(uint8_t{255}, destination[5]);
    }

    TEST_METHOD(write_two_comment) // NOLINT
    {
        jpegls_encoder encoder;

        array<uint8_t, 14> destination;
        encoder.destination(destination);

        encoder.write_comment("123");
        encoder.write_comment("");

        Assert::AreEqual(destination.size(), encoder.bytes_written());

        // Check that SOI marker has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that the COM segments have been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::comment), destination[3]);
        Assert::AreEqual(uint8_t{}, destination[4]);
        Assert::AreEqual(uint8_t{2 + 4}, destination[5]);
        Assert::AreEqual(uint8_t{'1'}, destination[6]);
        Assert::AreEqual(uint8_t{'2'}, destination[7]);
        Assert::AreEqual(uint8_t{'3'}, destination[8]);
        Assert::AreEqual(uint8_t{}, destination[9]);

        Assert::AreEqual(uint8_t{0xFF}, destination[10]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::comment), destination[11]);
        Assert::AreEqual(uint8_t{}, destination[12]);
        Assert::AreEqual(uint8_t{2}, destination[13]);
    }

    TEST_METHOD(write_too_large_comment_throws) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(2 + 2 + static_cast<size_t>(numeric_limits<uint16_t>::max()) + 1);
        encoder.destination(destination);

        constexpr size_t max_size_comment_data{static_cast<size_t>(numeric_limits<uint16_t>::max()) - 2};
        const vector<uint8_t> data(max_size_comment_data + 1);

        assert_expect_exception(jpegls_errc::invalid_argument_size,
                                [&encoder, &data] { ignore = encoder.write_comment(data.data(), data.size()); });
    }

    TEST_METHOD(write_comment_null_pointer_with_size_throws) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(100);
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_argument, [&encoder] {
            MSVC_WARNING_SUPPRESS_NEXT_LINE(6387) // argument> may be null.
            ignore = encoder.write_comment(nullptr, 1);
        });
    }

    TEST_METHOD(write_comment_after_encode_throws) // NOLINT
    {
        const vector<uint8_t> source{0, 1, 2, 3, 4, 5};

        jpegls_encoder encoder;

        vector<uint8_t> destination(100);
        encoder.destination(destination);
        encoder.frame_info({3, 1, 16, 1});
        ignore = encoder.encode(source);

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&encoder] { ignore = encoder.write_comment("after-encoding"); });
    }

    TEST_METHOD(write_comment_before_encode) // NOLINT
    {
        const vector<uint8_t> source{0, 1, 2, 3, 4, 5};
        constexpr frame_info frame_info{3, 1, 16, 1};

        jpegls_encoder encoder;
        vector<uint8_t> encoded(100);
        encoder.destination(encoded);
        encoder.frame_info(frame_info);

        encoder.write_comment("my comment");

        encoded.resize(encoder.encode(source));
        test_by_decoding(encoded, frame_info, source.data(), source.size(), interleave_mode::none);
    }

    TEST_METHOD(write_application_data) // NOLINT
    {
        jpegls_encoder encoder;

        array<uint8_t, 10> destination;
        encoder.destination(destination);

        const array<uint8_t, 4> application_data{1, 2, 3, 4};
        encoder.write_application_data(1, application_data.data(), application_data.size());

        Assert::AreEqual(size_t{10}, encoder.bytes_written());

        // Check that SOI marker has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that a APPn segment has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::application_data1), destination[3]);
        Assert::AreEqual(uint8_t{}, destination[4]);
        Assert::AreEqual(uint8_t{2 + 4}, destination[5]);
        Assert::AreEqual(uint8_t{1}, destination[6]);
        Assert::AreEqual(uint8_t{2}, destination[7]);
        Assert::AreEqual(uint8_t{3}, destination[8]);
        Assert::AreEqual(uint8_t{4}, destination[9]);
    }

    TEST_METHOD(write_empty_application_data) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(6);
        encoder.destination(destination);

        encoder.write_application_data(2, nullptr, 0);

        Assert::AreEqual(size_t{6}, encoder.bytes_written());

        // Check that SOI marker has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that a APPn segment has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::application_data2), destination[3]);
        Assert::AreEqual(uint8_t{}, destination[4]);
        Assert::AreEqual(uint8_t{2}, destination[5]);
    }

    TEST_METHOD(write_max_application_data) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(2 + 2 + static_cast<size_t>(numeric_limits<uint16_t>::max()));
        encoder.destination(destination);

        constexpr size_t max_size_application_data{static_cast<size_t>(numeric_limits<uint16_t>::max()) - 2};
        const vector<uint8_t> data(max_size_application_data);
        encoder.write_application_data(15, data.data(), data.size());

        Assert::AreEqual(destination.size(), encoder.bytes_written());

        // Check that SOI marker has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that a APPn segment has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::application_data15), destination[3]);
        Assert::AreEqual(uint8_t{255}, destination[4]);
        Assert::AreEqual(uint8_t{255}, destination[5]);
    }

    TEST_METHOD(write_two_application_data) // NOLINT
    {
        jpegls_encoder encoder;

        array<uint8_t, 14> destination;
        encoder.destination(destination);

        const array<uint8_t, 4> application_data{1, 2, 3, 4};
        encoder.write_application_data(0, application_data.data(), application_data.size());
        encoder.write_application_data(8, nullptr, 0);

        Assert::AreEqual(destination.size(), encoder.bytes_written());

        // Check that SOI marker has been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that the COM segments have been written.
        Assert::AreEqual(uint8_t{0xFF}, destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::application_data0), destination[3]);
        Assert::AreEqual(uint8_t{}, destination[4]);
        Assert::AreEqual(uint8_t{2 + 4}, destination[5]);
        Assert::AreEqual(uint8_t{1}, destination[6]);
        Assert::AreEqual(uint8_t{2}, destination[7]);
        Assert::AreEqual(uint8_t{3}, destination[8]);
        Assert::AreEqual(uint8_t{4}, destination[9]);

        Assert::AreEqual(uint8_t{0xFF}, destination[10]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::application_data8), destination[11]);
        Assert::AreEqual(uint8_t{}, destination[12]);
        Assert::AreEqual(uint8_t{2}, destination[13]);
    }

    TEST_METHOD(write_too_large_application_data_throws) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(2 + 2 + static_cast<size_t>(numeric_limits<uint16_t>::max()) + 1);
        encoder.destination(destination);

        constexpr size_t max_size_application_data{static_cast<size_t>(numeric_limits<uint16_t>::max()) - 2};
        const vector<uint8_t> data(max_size_application_data + 1);

        assert_expect_exception(jpegls_errc::invalid_argument_size,
                                [&encoder, &data] { ignore = encoder.write_application_data(0, data.data(), data.size()); });
    }

    TEST_METHOD(write_application_data_null_pointer_with_size_throws) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(100);
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_argument, [&encoder] {
            MSVC_WARNING_SUPPRESS_NEXT_LINE(6387)
            ignore = encoder.write_application_data(0, nullptr, 1);
        });
    }

    TEST_METHOD(write_application_data_after_encode_throws) // NOLINT
    {
        const vector<uint8_t> source{0, 1, 2, 3, 4, 5};

        jpegls_encoder encoder;

        vector<uint8_t> destination(100);
        encoder.destination(destination);
        encoder.frame_info({3, 1, 16, 1});
        ignore = encoder.encode(source);

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&encoder] { ignore = encoder.write_application_data(0, nullptr, 0); });
    }

    TEST_METHOD(write_application_data_with_bad_id_throws) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(100);
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_argument, [&encoder] {
            MSVC_WARNING_SUPPRESS_NEXT_LINE(6387)
            ignore = encoder.write_application_data(-1, nullptr, 0);
        });

        assert_expect_exception(jpegls_errc::invalid_argument, [&encoder] {
            MSVC_WARNING_SUPPRESS_NEXT_LINE(6387)
            ignore = encoder.write_application_data(16, nullptr, 0);
        });
    }

    TEST_METHOD(write_application_data_before_encode) // NOLINT
    {
        const vector<uint8_t> source{0, 1, 2, 3, 4, 5};
        constexpr frame_info frame_info{3, 1, 16, 1};

        jpegls_encoder encoder;
        vector<uint8_t> encoded(100);
        encoder.destination(encoded);
        encoder.frame_info(frame_info);

        encoder.write_application_data(11, nullptr, 0);

        encoded.resize(encoder.encode(source));
        test_by_decoding(encoded, frame_info, source.data(), source.size(), interleave_mode::none);
    }

    TEST_METHOD(set_preset_coding_parameters) // NOLINT
    {
        jpegls_encoder encoder;

        constexpr jpegls_pc_parameters pc_parameters{};
        encoder.preset_coding_parameters(pc_parameters);

        // No explicit test possible, code should remain stable.
        Assert::IsTrue(true);
    }

    TEST_METHOD(set_preset_coding_parameters_bad_values_throws) // NOLINT
    {
        const array<uint8_t, 5> source{0, 1, 1, 1, 0};
        constexpr frame_info frame_info{5, 1, 8, 1};
        jpegls_encoder encoder;

        encoder.frame_info(frame_info);
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        constexpr jpegls_pc_parameters bad_pc_parameters{1, 1, 1, 1, 1};
        encoder.preset_coding_parameters(bad_pc_parameters);

        assert_expect_exception(jpegls_errc::invalid_argument_jpegls_pc_parameters,
                                [&encoder, &source] { ignore = encoder.encode(source); });
    }

    TEST_METHOD(encode_with_preset_coding_parameters_non_default_values) // NOLINT
    {
        encode_with_custom_preset_coding_parameters({1, 0, 0, 0, 0});
        encode_with_custom_preset_coding_parameters({0, 1, 0, 0, 0});
        encode_with_custom_preset_coding_parameters({0, 0, 4, 0, 0});
        encode_with_custom_preset_coding_parameters({0, 0, 0, 8, 0});
        encode_with_custom_preset_coding_parameters({0, 1, 2, 3, 0});
        encode_with_custom_preset_coding_parameters({0, 0, 0, 0, 63});
    }

    TEST_METHOD(set_color_transformation_bad_value_throws) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_color_transformation,
                                [&encoder] { encoder.color_transformation(static_cast<color_transformation>(100)); });
    }

    TEST_METHOD(encode_without_destination_throws) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});
        vector<uint8_t> source(20);
        assert_expect_exception(jpegls_errc::invalid_operation, [&encoder, &source] { ignore = encoder.encode(source); });
    }

    TEST_METHOD(encode_without_frame_info_throws) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(20);
        encoder.destination(destination);
        const vector<uint8_t> source(20);
        assert_expect_exception(jpegls_errc::invalid_operation, [&encoder, &source] { ignore = encoder.encode(source); });
    }

    TEST_METHOD(encode_with_spiff_header) // NOLINT
    {
        const array<uint8_t, 5> source{0, 1, 2, 3, 4};
        constexpr frame_info frame_info{5, 1, 8, 1};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        encoder.write_standard_spiff_header(spiff_color_space::grayscale);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        test_by_decoding(destination, frame_info, source.data(), source.size(), interleave_mode::none);
    }

    TEST_METHOD(encode_with_color_transformation) // NOLINT
    {
        const array<uint8_t, 6> source{0, 1, 2, 3, 4, 5};
        constexpr frame_info frame_info{2, 1, 8, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination).color_transformation(color_transformation::hp1);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        test_by_decoding(destination, frame_info, source.data(), source.size(), interleave_mode::none,
                         color_transformation::hp1);
    }

    TEST_METHOD(encode_16_bit) // NOLINT
    {
        const array<uint8_t, 6> source{0, 1, 2, 3, 4, 5};
        constexpr frame_info frame_info{3, 1, 16, 1};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        test_by_decoding(destination, frame_info, source.data(), source.size(), interleave_mode::none);
    }

    TEST_METHOD(simple_encode) // NOLINT
    {
        const vector<uint8_t> source{0, 1, 2, 3, 4, 5};

        constexpr frame_info frame_info{3, 1, 16, 1};
        const auto encoded{jpegls_encoder::encode(source, frame_info)};

        test_by_decoding(encoded, frame_info, source.data(), source.size(), interleave_mode::none);
    }

    TEST_METHOD(encode_with_stride_interleave_none_8_bit) // NOLINT
    {
        const array<uint8_t, 30> source{100, 100, 100, 0, 0, 0, 0, 0, 0,   0,   150, 150,
                                        150, 0,   0,   0, 0, 0, 0, 0, 200, 200, 200};
        constexpr frame_info frame_info{3, 1, 8, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source, 10)};
        destination.resize(bytes_written);

        const array<uint8_t, 9> expected{100, 100, 100, 150, 150, 150, 200, 200, 200};
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::none);
    }

    TEST_METHOD(encode_with_stride_interleave_none_16_bit) // NOLINT
    {
        const array<uint16_t, 30> source{100, 100, 100, 0, 0, 0, 0, 0, 0,   0,   150, 150,
                                         150, 0,   0,   0, 0, 0, 0, 0, 200, 200, 200};
        constexpr frame_info frame_info{3, 1, 16, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source, 10 * sizeof(uint16_t))};
        destination.resize(bytes_written);

        const array<uint16_t, 9> expected{100, 100, 100, 150, 150, 150, 200, 200, 200};
        test_by_decoding(destination, frame_info, expected.data(), expected.size() * sizeof(uint16_t),
                         interleave_mode::none);
    }

    TEST_METHOD(encode_with_stride_interleave_sample_8_bit) // NOLINT
    {
        const array<uint8_t, 10> source{100, 150, 200, 100, 150, 200, 100, 150, 200, 0};
        constexpr frame_info frame_info{3, 1, 8, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::sample);
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source, 10)};
        destination.resize(bytes_written);

        const array<uint8_t, 9> expected{100, 150, 200, 100, 150, 200, 100, 150, 200};
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::sample);
    }

    TEST_METHOD(encode_with_stride_interleave_sample_16_bit) // NOLINT
    {
        const array<uint16_t, 10> source{100, 150, 200, 100, 150, 200, 100, 150, 200, 0};
        constexpr frame_info frame_info{3, 1, 16, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::sample);
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source, 10 * sizeof(uint16_t))};
        destination.resize(bytes_written);

        const array<uint16_t, 9> expected{100, 150, 200, 100, 150, 200, 100, 150, 200};
        test_by_decoding(destination, frame_info, expected.data(), expected.size() * sizeof(uint16_t),
                         interleave_mode::sample);
    }

    TEST_METHOD(encode_with_bad_stride_interleave_none_throws) // NOLINT
    {
        const array<uint8_t, 29> source{100, 100, 100, 0, 0, 0, 0, 0, 0,   0,   150, 150,
                                        150, 0,   0,   0, 0, 0, 0, 0, 200, 200, 200};
        constexpr frame_info frame_info{3, 1, 8, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_argument_stride,
                                [&encoder, &source] { ignore = encoder.encode(source, 10); });
    }

    TEST_METHOD(encode_with_bad_stride_interleave_sample_throws) // NOLINT
    {
        const array<uint8_t, 9> source{100, 150, 200, 100, 150, 200, 100, 150, 200};
        constexpr frame_info frame_info{3, 1, 8, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::sample);
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_argument_stride,
                                [&encoder, &source] { ignore = encoder.encode(source, 10); });
    }

    TEST_METHOD(encode_1_component_4_bit_with_high_bits_set) // NOLINT
    {
        const vector<uint8_t> source(size_t{512} * 512, 0xFF);
        constexpr frame_info frame_info{512, 512, 4, 1};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint8_t> expected(size_t{512} * 512, 15);
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::none);
    }

    TEST_METHOD(encode_1_component_12_bit_with_high_bits_set) // NOLINT
    {
        const vector<uint8_t> source(size_t{512} * 512 * 2, 0xFF);
        constexpr frame_info frame_info{512, 512, 12, 1};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint16_t> expected(size_t{512} * 512, 4095);
        test_by_decoding(destination, frame_info, expected.data(), expected.size() * sizeof(uint16_t),
                         interleave_mode::none);
    }

    TEST_METHOD(encode_3_components_6_bit_with_high_bits_set_interleave_mode_sample) // NOLINT
    {
        const vector<uint8_t> source(size_t{512} * 512 * 3, 0xFF);
        constexpr frame_info frame_info{512, 512, 6, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::sample);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint8_t> expected(size_t{512} * 512 * 3, 63);
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::sample);
    }

    TEST_METHOD(encode_3_components_6_bit_with_high_bits_set_interleave_mode_line) // NOLINT
    {
        const vector<uint8_t> source(size_t{512} * 512 * 3, 0xFF);
        constexpr frame_info frame_info{512, 512, 6, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::line);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint8_t> expected(size_t{512} * 512 * 3, 63);
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::line);
    }

    TEST_METHOD(encode_3_components_10_bit_with_high_bits_set_interleave_mode_sample) // NOLINT
    {
        const vector<uint8_t> source(size_t{512} * 512 * 2 * 3, 0xFF);
        constexpr frame_info frame_info{512, 512, 10, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::sample);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint16_t> expected(size_t{512} * 512 * 3, 1023);
        test_by_decoding(destination, frame_info, expected.data(), expected.size() * 2, interleave_mode::sample);
    }

    TEST_METHOD(encode_3_components_10_bit_with_high_bits_set_interleave_mode_line) // NOLINT
    {
        const vector<uint8_t> source(size_t{512} * 512 * 2 * 3, 0xFF);
        constexpr frame_info frame_info{512, 512, 10, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::line);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint16_t> expected(size_t{512} * 512 * 3, 1023);
        test_by_decoding(destination, frame_info, expected.data(), expected.size() * 2, interleave_mode::line);
    }

    TEST_METHOD(encode_4_components_6_bit_with_high_bits_set_interleave_mode_sample) // NOLINT
    {
        const vector<uint8_t> source(size_t{512} * 512 * 4, 0xFF);
        constexpr frame_info frame_info{512, 512, 6, 4};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::sample);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint8_t> expected(size_t{512} * 512 * 4, 63);
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::sample);
    }

    TEST_METHOD(encode_4_components_6_bit_with_high_bits_set_interleave_mode_line) // NOLINT
    {
        const vector<uint8_t> source(size_t{512} * 512 * 4, 0xFF);
        constexpr frame_info frame_info{512, 512, 6, 4};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::line);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint8_t> expected(size_t{512} * 512 * 4, 63);
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::line);
    }

    TEST_METHOD(encode_4_components_10_bit_with_high_bits_set_interleave_mode_sample) // NOLINT
    {
        const vector<uint8_t> source(size_t{512} * 512 * 2 * 4, 0xFF);
        constexpr frame_info frame_info{512, 512, 10, 4};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::sample);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint16_t> expected(size_t{512} * 512 * 4, 1023);
        test_by_decoding(destination, frame_info, expected.data(), expected.size() * 2, interleave_mode::sample);
    }

    TEST_METHOD(encode_4_components_10_bit_with_high_bits_set_interleave_mode_line) // NOLINT
    {
        const vector<uint8_t> source(size_t{512} * 512 * 2 * 4, 0xFF);
        constexpr frame_info frame_info{512, 512, 10, 4};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::line);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint16_t> expected(size_t{512} * 512 * 4, 1023);
        test_by_decoding(destination, frame_info, expected.data(), expected.size() * 2, interleave_mode::line);
    }

    TEST_METHOD(rewind) // NOLINT
    {
        const array<uint8_t, 6> source{0, 1, 2, 3, 4, 5};
        constexpr frame_info frame_info{3, 1, 16, 1};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written1{encoder.encode(source)};
        destination.resize(bytes_written1);

        test_by_decoding(destination, frame_info, source.data(), source.size(), interleave_mode::none);

        const vector<uint8_t> destination_backup(destination);

        encoder.rewind();
        const size_t bytes_written2{encoder.encode(source)};

        Assert::AreEqual(bytes_written1, bytes_written2);
        Assert::IsTrue(destination_backup == destination);
    }

    TEST_METHOD(rewind_before_destination) // NOLINT
    {
        const array<uint8_t, 6> source{0, 1, 2, 3, 4, 5};
        constexpr frame_info frame_info{3, 1, 16, 1};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.rewind();
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        test_by_decoding(destination, frame_info, source.data(), source.size(), interleave_mode::none);
    }

    TEST_METHOD(encode_image_odd_size) // NOLINT
    {
        constexpr frame_info frame_info{512, 512, 8, 1};
        const vector<uint8_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        const auto destination{jpegls_encoder::encode(source, frame_info)};

        Assert::AreEqual(size_t{99}, destination.size());
        test_by_decoding(destination, frame_info, source.data(), source.size(), interleave_mode::none);
    }

    TEST_METHOD(encode_image_odd_size_forced_even) // NOLINT
    {
        constexpr frame_info frame_info{512, 512, 8, 1};
        const vector<uint8_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        const auto destination{
            jpegls_encoder::encode(source, frame_info, interleave_mode::none, encoding_options::even_destination_size)};

        Assert::AreEqual(size_t{100}, destination.size());
        test_by_decoding(destination, frame_info, source.data(), source.size(), interleave_mode::none);
    }

    TEST_METHOD(encode_image_forced_version_comment) // NOLINT
    {
        constexpr frame_info frame_info{512, 512, 8, 1};
        const vector<uint8_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        const auto encoded_source{
            jpegls_encoder::encode(source, frame_info, interleave_mode::none, encoding_options::include_version_number)};

        jpegls_decoder decoder;
        decoder.source(encoded_source);

        const char* actual_data{};
        size_t actual_size{};
        decoder.at_comment([&actual_data, &actual_size](const void* data, const size_t size) noexcept {
            actual_data = static_cast<const char*>(data);
            actual_size = size;
        });

        decoder.read_header();

        const std::string expected{"charls "s + charls_get_version_string()};

        Assert::AreEqual(expected.size() + 1, actual_size);
        Assert::IsTrue(memcmp(expected.data(), actual_data, actual_size) == 0);
    }

    TEST_METHOD(encode_image_include_pc_parameters_jai) // NOLINT
    {
        constexpr frame_info frame_info{1, 1, 16, 1};
        const vector<uint16_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        // Note: encoding_options::include_pc_parameters_jai is enabled by default (until the next major version)

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        Assert::AreEqual(size_t{43}, bytes_written);

        Assert::AreEqual(uint8_t{0xFF}, destination[15]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::jpegls_preset_parameters), destination[16]);

        // Segment size.
        Assert::AreEqual(uint8_t{}, destination[17]);
        Assert::AreEqual(uint8_t{13}, destination[18]);

        // Parameter ID.
        Assert::AreEqual(uint8_t{0x1}, destination[19]);

        // MaximumSampleValue
        Assert::AreEqual(uint8_t{255}, destination[20]);
        Assert::AreEqual(uint8_t{255}, destination[21]);

        constexpr thresholds expected{
            compute_defaults_using_reference_implementation(std::numeric_limits<uint16_t>::max(), 0)};

        const int32_t threshold1{destination[22] << 8 | destination[23]};
        Assert::AreEqual(expected.t1, threshold1);

        const int32_t threshold2{destination[24] << 8 | destination[25]};
        Assert::AreEqual(expected.t2, threshold2);

        const int32_t threshold3{destination[26] << 8 | destination[27]};
        Assert::AreEqual(expected.t3, threshold3);

        const int32_t reset{destination[28] << 8 | destination[29]};
        Assert::AreEqual(expected.reset, reset);
    }

    TEST_METHOD(encode_image_with_disabled_include_pc_parameters_jai) // NOLINT
    {
        constexpr frame_info frame_info{1, 1, 16, 1};
        const vector<uint16_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.encoding_options(encoding_options::none);

        const size_t bytes_written{encoder.encode(source)};

        Assert::AreEqual(size_t{28}, bytes_written);
    }

    TEST_METHOD(set_invalid_encode_options_throws) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_encoding_options,
                                [&encoder] { encoder.encoding_options(static_cast<encoding_options>(8)); });
    }

    TEST_METHOD(large_image_contains_lse_for_oversize_image_dimension) // NOLINT
    {
        constexpr frame_info frame_info{numeric_limits<uint16_t>::max() + 1, 1, 16, 1};
        const vector<uint16_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        Assert::AreEqual(size_t{61}, bytes_written);

        destination.resize(bytes_written);
        const auto it{find_first_lse_segment(destination.cbegin(), destination.cend())};
        Assert::IsTrue(it != destination.cend());
    }

    TEST_METHOD(encode_oversized_image) // NOLINT
    {
        constexpr frame_info frame_info{numeric_limits<uint16_t>::max() + 1, 1, 8, 1};
        const vector<uint8_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        const auto encoded_source{jpegls_encoder::encode(source, frame_info)};

        test_by_decoding(encoded_source, frame_info, source.data(), source.size(), interleave_mode::none);
    }

    TEST_METHOD(image_contains_no_preset_coding_parameters_by_default) // NOLINT
    {
        constexpr frame_info frame_info{512, 512, 8, 1};
        const vector<uint8_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        Assert::AreEqual(size_t{99}, bytes_written);

        destination.resize(bytes_written);
        const auto it{find_first_lse_segment(destination.cbegin(), destination.cend())};
        Assert::IsTrue(it == destination.cend());
    }

    TEST_METHOD(image_contains_no_preset_coding_parameters_if_configured_pc_is_default) // NOLINT
    {
        constexpr frame_info frame_info{512, 512, 8, 1};
        const vector<uint8_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).preset_coding_parameters({255, 3, 7, 21, 64});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        Assert::AreEqual(size_t{99}, bytes_written);

        destination.resize(bytes_written);
        const auto it{find_first_lse_segment(destination.cbegin(), destination.cend())};
        Assert::IsTrue(it == destination.cend());
    }

    TEST_METHOD(image_contains_preset_coding_parameters_if_configured_pc_is_non_default) // NOLINT
    {
        constexpr frame_info frame_info{512, 512, 8, 1};
        const vector<uint8_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).preset_coding_parameters({255, 3, 7, 21, 65});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        Assert::AreEqual(size_t{114}, bytes_written);

        destination.resize(bytes_written);
        const auto it{find_first_lse_segment(destination.cbegin(), destination.cend())};
        Assert::IsFalse(it == destination.cend());
    }

    TEST_METHOD(image_contains_preset_coding_parameters_if_configured_pc_has_diff_max_value) // NOLINT
    {
        constexpr frame_info frame_info{512, 512, 8, 1};
        const vector<uint8_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).preset_coding_parameters({100, 0, 0, 0, 0});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        Assert::AreEqual(size_t{114}, bytes_written);

        destination.resize(bytes_written);
        const auto it{find_first_lse_segment(destination.cbegin(), destination.cend())};
        Assert::IsFalse(it == destination.cend());
    }

    TEST_METHOD(encode_to_buffer_with_uint16_size_works) // NOLINT
    {
        // These are compile time checks to detect issues with overloads that have similar conversions.
        constexpr frame_info frame_info{100, 100, 8, 1};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);

        vector<uint8_t> destination(encoder.estimated_destination_size());

        void* data1{destination.data()};
        const auto size1{static_cast<uint16_t>(destination.size())};
        encoder.destination(data1, size1);

        vector<uint8_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);
        void* data2{source.data()};
        const auto size2{static_cast<uint16_t>(source.size())};

        // Set 1 value to prevent complains about const.
        auto* p{static_cast<uint8_t*>(data2)};
        *p = 7;

        // size2 is not a perfect match and needs a conversion.
        ignore = encoder.encode(data2, size2);
    }

private:
    static void test_by_decoding(const vector<uint8_t>& encoded_source, const frame_info& source_frame_info,
                                 const void* expected_destination, const size_t expected_destination_size,
                                 const charls::interleave_mode interleave_mode,
                                 const color_transformation color_transformation = color_transformation::none)
    {
        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        const auto& frame_info{decoder.frame_info()};
        Assert::AreEqual(source_frame_info.width, frame_info.width);
        Assert::AreEqual(source_frame_info.height, frame_info.height);
        Assert::AreEqual(source_frame_info.bits_per_sample, frame_info.bits_per_sample);
        Assert::AreEqual(source_frame_info.component_count, frame_info.component_count);
        Assert::IsTrue(interleave_mode == decoder.interleave_mode());
        Assert::IsTrue(color_transformation == decoder.color_transformation());

        vector<uint8_t> destination(decoder.destination_size());
        decoder.decode(destination);

        Assert::AreEqual(destination.size(), expected_destination_size);

        if (decoder.near_lossless() == 0)
        {
            const auto* expected_destination_byte{static_cast<const uint8_t*>(expected_destination)};

            for (size_t i{}; i != expected_destination_size; ++i)
            {
                if (expected_destination_byte[i] != destination[i]) // AreEqual is very slow, pre-test to speed up 50X
                {
                    Assert::AreEqual(expected_destination_byte[i], destination[i]);
                }
            }
        }
    }

    static void encode_with_custom_preset_coding_parameters(const jpegls_pc_parameters& pc_parameters)
    {
        const array<uint8_t, 5> source{0, 1, 1, 1, 0};
        constexpr frame_info frame_info{5, 1, 8, 1};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        encoder.preset_coding_parameters(pc_parameters);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        test_by_decoding(destination, frame_info, source.data(), source.size(), interleave_mode::none);
    }

    static vector<uint8_t>::const_iterator find_first_lse_segment(const vector<uint8_t>::const_iterator begin,
                                                                  const vector<uint8_t>::const_iterator end) noexcept
    {
        constexpr uint8_t lse_marker{0xF8};

        for (auto it{begin}; it != end; ++it)
        {
            if (*it == 0xFF && it + 1 != end && *(it + 1) == lse_marker)
                return it;
        }

        return end;
    }
};

}} // namespace charls::test
