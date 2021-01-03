// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include "../src/jpeg_marker_code.h"
#include <charls/charls.h>

#include <array>
#include <vector>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;
using std::vector;

constexpr size_t serialized_spiff_header_size = 34;

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

        encoder.frame_info({1, 1, 2, 1});                      // minimum.
        encoder.frame_info({UINT16_MAX, UINT16_MAX, 16, 255}); // maximum.
    }

    TEST_METHOD(frame_info_bad_width) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_width, [&] { encoder.frame_info({0, 1, 2, 1}); });
        assert_expect_exception(jpegls_errc::invalid_argument_width, [&] { encoder.frame_info({UINT16_MAX + 1, 1, 2, 1}); });
    }

    TEST_METHOD(frame_info_bad_height) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_height, [&] { encoder.frame_info({1, 0, 2, 1}); });
        assert_expect_exception(jpegls_errc::invalid_argument_height, [&] {
            encoder.frame_info({1, UINT16_MAX + 1, 2, 1});
        });
    }

    TEST_METHOD(frame_info_bad_bits_per_sample) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_bits_per_sample, [&] { encoder.frame_info({1, 1, 1, 1}); });
        assert_expect_exception(jpegls_errc::invalid_argument_bits_per_sample, [&] { encoder.frame_info({1, 1, 17, 1}); });
    }

    TEST_METHOD(frame_info_bad_component_count) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_component_count, [&] { encoder.frame_info({1, 1, 2, 0}); });
        assert_expect_exception(jpegls_errc::invalid_argument_component_count, [&] { encoder.frame_info({1, 1, 2, 256}); });
    }

    TEST_METHOD(interleave_mode) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.interleave_mode(interleave_mode::none);
        encoder.interleave_mode(interleave_mode::line);
        encoder.interleave_mode(interleave_mode::sample);
    }

    TEST_METHOD(interleave_mode_bad) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_interleave_mode,
                                [&] { encoder.interleave_mode(static_cast<charls::interleave_mode>(-1)); });
        assert_expect_exception(jpegls_errc::invalid_argument_interleave_mode,
                                [&] { encoder.interleave_mode(static_cast<charls::interleave_mode>(3)); });
    }

    TEST_METHOD(near_lossless) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.near_lossless(0);   // set lowest value.
        encoder.near_lossless(255); // set highest value.
    }

    TEST_METHOD(near_lossless_bad) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_near_lossless, [&] { encoder.near_lossless(-1); });
        assert_expect_exception(jpegls_errc::invalid_argument_near_lossless, [&] { encoder.near_lossless(256); });
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

        encoder.frame_info({UINT16_MAX, UINT16_MAX, 8, 1}); // = maximum.
        const auto size{encoder.estimated_destination_size()};
        constexpr auto expected{static_cast<size_t>(UINT16_MAX) * UINT16_MAX * 1 * 1};
        Assert::IsTrue(size >= expected);
    }

    TEST_METHOD(estimated_destination_size_monochrome_16_bit) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({100, 100, 16, 1}); // minimum.
        const auto size{encoder.estimated_destination_size()};
        Assert::IsTrue(size >= 100 * 100 * 2);
    }

    TEST_METHOD(estimated_destination_size_color_8_bit) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({2000, 2000, 8, 3});
        const auto size{encoder.estimated_destination_size()};
        Assert::IsTrue(size >= 2000 * 2000 * 3);
    }

    TEST_METHOD(estimated_destination_size_very_wide) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({UINT16_MAX, 1, 8, 1});
        const auto size{encoder.estimated_destination_size()};
        Assert::IsTrue(size >= UINT16_MAX + 1024);
    }

    TEST_METHOD(estimated_destination_size_very_high) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, UINT16_MAX, 8, 1});
        const auto size{encoder.estimated_destination_size()};
        Assert::IsTrue(size >= UINT16_MAX + 1024);
    }

    TEST_METHOD(estimated_destination_size_too_soon) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&] { static_cast<void>(encoder.estimated_destination_size()); });
    }

    TEST_METHOD(destination) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(200);
        encoder.destination(destination);
    }

    TEST_METHOD(destination_can_only_be_set_once) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(200);
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_operation, [&] { encoder.destination(destination); });
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
        Assert::AreEqual(static_cast<uint8_t>(0xFF), destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that a APP8 with SPIFF has been written (details already verified by jpeg_stream_writer_test).
        Assert::AreEqual(static_cast<uint8_t>(0xFF), destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::application_data8), destination[3]);
        Assert::AreEqual(static_cast<uint8_t>(0), destination[4]);
        Assert::AreEqual(static_cast<uint8_t>(32), destination[5]);
        Assert::AreEqual(static_cast<uint8_t>('S'), destination[6]);
        Assert::AreEqual(static_cast<uint8_t>('P'), destination[7]);
        Assert::AreEqual(static_cast<uint8_t>('I'), destination[8]);
        Assert::AreEqual(static_cast<uint8_t>('F'), destination[9]);
        Assert::AreEqual(static_cast<uint8_t>('F'), destination[10]);
        Assert::AreEqual(static_cast<uint8_t>(0), destination[11]);
    }

    TEST_METHOD(write_standard_spiff_header_without_destination) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&] { encoder.write_standard_spiff_header(spiff_color_space::cmyk); });
    }

    TEST_METHOD(write_standard_spiff_header_without_frame_info) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(100);
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&] { encoder.write_standard_spiff_header(spiff_color_space::cmyk); });
    }

    TEST_METHOD(write_standard_spiff_header_twice) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&] { encoder.write_standard_spiff_header(spiff_color_space::cmyk); });
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
        Assert::AreEqual(static_cast<uint8_t>(0xFF), destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), destination[1]);

        // Verify that a APP8 with SPIFF has been written (details already verified by jpeg_stream_writer_test).
        Assert::AreEqual(static_cast<uint8_t>(0xFF), destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::application_data8), destination[3]);
        Assert::AreEqual(static_cast<uint8_t>(0), destination[4]);
        Assert::AreEqual(static_cast<uint8_t>(32), destination[5]);
        Assert::AreEqual(static_cast<uint8_t>('S'), destination[6]);
        Assert::AreEqual(static_cast<uint8_t>('P'), destination[7]);
        Assert::AreEqual(static_cast<uint8_t>('I'), destination[8]);
        Assert::AreEqual(static_cast<uint8_t>('F'), destination[9]);
        Assert::AreEqual(static_cast<uint8_t>('F'), destination[10]);
        Assert::AreEqual(static_cast<uint8_t>(0), destination[11]);
    }

    TEST_METHOD(write_spiff_header_invalid_height) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        spiff_header spiff_header{};
        spiff_header.width = 1;

        assert_expect_exception(jpegls_errc::invalid_argument_height, [&] { encoder.write_spiff_header(spiff_header); });
        Assert::AreEqual(static_cast<size_t>(0), encoder.bytes_written());
    }

    TEST_METHOD(write_spiff_header_invalid_width) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        spiff_header spiff_header{};
        spiff_header.height = 1;

        assert_expect_exception(jpegls_errc::invalid_argument_width, [&] { encoder.write_spiff_header(spiff_header); });
        Assert::AreEqual(static_cast<size_t>(0), encoder.bytes_written());
    }

    TEST_METHOD(write_spiff_entry) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        encoder.write_spiff_entry(spiff_entry_tag::image_title, "test", 4);

        Assert::AreEqual(static_cast<size_t>(48), encoder.bytes_written());
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

        Assert::AreEqual(static_cast<size_t>(60), encoder.bytes_written());
    }

    TEST_METHOD(write_empty_spiff_entry) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        encoder.write_spiff_entry(spiff_entry_tag::image_title, nullptr, 0);

        Assert::AreEqual(static_cast<size_t>(44), encoder.bytes_written());
    }

    TEST_METHOD(write_spiff_entry_with_invalid_tag) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        assert_expect_exception(jpegls_errc::invalid_argument, [&] { encoder.write_spiff_entry(1, "test", 4); });
    }

    TEST_METHOD(write_spiff_entry_with_invalid_size) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        assert_expect_exception(jpegls_errc::invalid_argument_spiff_entry_size, [&] {
            vector<uint8_t> spiff_entry(65528 + 1);
            encoder.write_spiff_entry(spiff_entry_tag::image_title, spiff_entry.data(), spiff_entry.size());
        });
    }

    TEST_METHOD(write_spiff_entry_without_spiff_header) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_operation, [&] {
            vector<uint8_t> spiff_entry(65528);
            encoder.write_spiff_entry(spiff_entry_tag::image_title, spiff_entry.data(), spiff_entry.size());
        });
    }

    TEST_METHOD(set_preset_coding_parameters) // NOLINT
    {
        jpegls_encoder encoder;

        const charls_jpegls_pc_parameters pc_parameters{};
        encoder.preset_coding_parameters(pc_parameters);

        // No explicit test possible, code should remain stable.
        Assert::IsTrue(true);
    }

    TEST_METHOD(set_preset_coding_parameters_bad_values) // NOLINT
    {
        jpegls_encoder encoder;

        charls_jpegls_pc_parameters pc_parameters{1, 1, 1, 1, 1};

        assert_expect_exception(jpegls_errc::invalid_argument_jpegls_pc_parameters,
                                [&] { encoder.preset_coding_parameters(pc_parameters); });
    }

    TEST_METHOD(set_color_transformation_bad_value) // NOLINT
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_color_transformation,
                                [&] { encoder.color_transformation(static_cast<color_transformation>(100)); });
    }

    TEST_METHOD(encode_without_destination) // NOLINT
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});
        vector<uint8_t> source(20);
        assert_expect_exception(jpegls_errc::invalid_operation, [&] { static_cast<void>(encoder.encode(source)); });
    }

    TEST_METHOD(encode_without_frame_info) // NOLINT
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(20);
        encoder.destination(destination);
        vector<uint8_t> source(20);
        assert_expect_exception(jpegls_errc::invalid_operation, [&] { static_cast<void>(encoder.encode(source)); });
    }

    TEST_METHOD(encode_with_spiff_header) // NOLINT
    {
        const array<uint8_t, 5> source{0, 1, 2, 3, 4};
        const frame_info frame_info{5, 1, 8, 1};

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
        const frame_info frame_info{2, 1, 8, 3};

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
        const frame_info frame_info{3, 1, 16, 1};

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

        const frame_info frame_info{3, 1, 16, 1};
        const auto encoded{jpegls_encoder::encode(source, frame_info)};

        test_by_decoding(encoded, frame_info, source.data(), source.size(), interleave_mode::none);
    }

    TEST_METHOD(encode_with_stride) // NOLINT
    {
        const array<uint8_t, 30> source{100, 100, 100, 0, 0, 0, 0, 0, 0,   0,   150, 150,
                                        150, 0,   0,   0, 0, 0, 0, 0, 200, 200, 200};
        const frame_info frame_info{3, 1, 8, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source, 10)};
        destination.resize(bytes_written);

        const array<uint8_t, 9> expected{100, 100, 100, 150, 150, 150, 200, 200, 200};
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::none);
    }

    TEST_METHOD(encode_1_component_4_bit_with_high_bits_set) // NOLINT
    {
        const vector<uint8_t> source(512 * 512, 0xFF);
        const frame_info frame_info{512, 512, 4, 1};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint8_t> expected(512 * 512, 15);
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::none);
    }

    TEST_METHOD(encode_1_component_12_bit_with_high_bits_set) // NOLINT
    {
        const vector<uint8_t> source(512 * 512 * 2, 0xFF);
        const frame_info frame_info{512, 512, 12, 1};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint16_t> expected(512 * 512, 4095);
        test_by_decoding(destination, frame_info, expected.data(), expected.size() * 2, interleave_mode::none);
    }

    TEST_METHOD(encode_3_components_6_bit_with_high_bits_set_interleave_mode_sample) // NOLINT
    {
        const vector<uint8_t> source(512 * 512 * 3, 0xFF);
        const frame_info frame_info{512, 512, 6, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::sample);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint8_t> expected(512 * 512 * 3, 63);
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::sample);
    }

    TEST_METHOD(encode_3_components_6_bit_with_high_bits_set_interleave_mode_line) // NOLINT
    {
        const vector<uint8_t> source(512 * 512 * 3, 0xFF);
        const frame_info frame_info{512, 512, 6, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::line);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint8_t> expected(512 * 512 * 3, 63);
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::line);
    }

    TEST_METHOD(encode_3_components_10_bit_with_high_bits_set_interleave_mode_sample) // NOLINT
    {
        const vector<uint8_t> source(512 * 512 * 2 * 3, 0xFF);
        const frame_info frame_info{512, 512, 10, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::sample);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint16_t> expected(512 * 512 * 3, 1023);
        test_by_decoding(destination, frame_info, expected.data(), expected.size() * 2, interleave_mode::sample);
    }

    TEST_METHOD(encode_3_components_10_bit_with_high_bits_set_interleave_mode_line) // NOLINT
    {
        const vector<uint8_t> source(512 * 512 * 2 * 3, 0xFF);
        const frame_info frame_info{512, 512, 10, 3};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::line);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint16_t> expected(512 * 512 * 3, 1023);
        test_by_decoding(destination, frame_info, expected.data(), expected.size() * 2, interleave_mode::line);
    }

    TEST_METHOD(encode_4_components_6_bit_with_high_bits_set_interleave_mode_sample) // NOLINT
    {
        const vector<uint8_t> source(512 * 512 * 4, 0xFF);
        const frame_info frame_info{512, 512, 6, 4};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::sample);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint8_t> expected(512 * 512 * 4, 63);
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::sample);
    }

    TEST_METHOD(encode_4_components_6_bit_with_high_bits_set_interleave_mode_line) // NOLINT
    {
        const vector<uint8_t> source(512 * 512 * 4, 0xFF);
        const frame_info frame_info{512, 512, 6, 4};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::line);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint8_t> expected(512 * 512 * 4, 63);
        test_by_decoding(destination, frame_info, expected.data(), expected.size(), interleave_mode::line);
    }

    TEST_METHOD(encode_4_components_10_bit_with_high_bits_set_interleave_mode_sample) // NOLINT
    {
        const vector<uint8_t> source(512 * 512 * 2 * 4, 0xFF);
        const frame_info frame_info{512, 512, 10, 4};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::sample);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint16_t> expected(512 * 512 * 4, 1023);
        test_by_decoding(destination, frame_info, expected.data(), expected.size() * 2, interleave_mode::sample);
    }

    TEST_METHOD(encode_4_components_10_bit_with_high_bits_set_interleave_mode_line) // NOLINT
    {
        const vector<uint8_t> source(512 * 512 * 2 * 4, 0xFF);
        const frame_info frame_info{512, 512, 10, 4};

        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode::line);

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        const vector<uint16_t> expected(512 * 512 * 4, 1023);
        test_by_decoding(destination, frame_info, expected.data(), expected.size() * 2, interleave_mode::line);
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

        const auto& frame_info = decoder.frame_info();
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

            for (size_t i{}; i < expected_destination_size; ++i)
            {
                if (expected_destination_byte[i] != destination[i]) // AreEqual is very slow, pre-test to speed up 50X
                {
                    Assert::AreEqual(expected_destination_byte[i], destination[i]);
                }
            }
        }
    }
};

}} // namespace charls::test
