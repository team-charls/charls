// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include <vector>
#include <array>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using namespace charls;
using std::vector;
using std::array;

namespace CharLSUnitTest {

// clang-format off

TEST_CLASS(jpegls_encoder_test)
{
public:
    TEST_METHOD(create_destroy)
    {
        // ReSharper disable once CppLocalVariableWithNonTrivialDtorIsNeverUsed
        jpegls_encoder encoder;
    }

    TEST_METHOD(create_and_move)
    {
        jpegls_encoder encoder1;

        jpegls_encoder encoder2(std::move(encoder1));

        // ReSharper disable once CppLocalVariableWithNonTrivialDtorIsNeverUsed
        jpegls_encoder encoder3 = std::move(encoder2);
    }

    TEST_METHOD(frame_info)
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1}); // minimum.
        encoder.frame_info({UINT16_MAX, UINT16_MAX, 16, 255}); // maximum.
    }

    TEST_METHOD(frame_info_bad_width)
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_width, [&] { encoder.frame_info({0, 1, 2, 1}); });
        assert_expect_exception(jpegls_errc::invalid_argument_width, [&] { encoder.frame_info({UINT16_MAX + 1, 1, 2, 1}); });
    }

    TEST_METHOD(frame_info_bad_height)
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_height, [&] { encoder.frame_info({1, 0, 2, 1}); });
        assert_expect_exception(jpegls_errc::invalid_argument_height, [&] { encoder.frame_info({1, UINT16_MAX + 1, 2, 1}); });
    }

    TEST_METHOD(frame_info_bad_bits_per_sample)
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_bits_per_sample, [&] { encoder.frame_info({1, 1, 1, 1}); });
        assert_expect_exception(jpegls_errc::invalid_argument_bits_per_sample, [&] { encoder.frame_info({1, 1, 17, 1}); });
    }

    TEST_METHOD(frame_info_bad_component_count)
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_component_count, [&] { encoder.frame_info({1, 1, 2, 0}); });
        assert_expect_exception(jpegls_errc::invalid_argument_component_count, [&] { encoder.frame_info({1, 1, 2, 256}); });
    }

    TEST_METHOD(interleave_mode)
    {
        jpegls_encoder encoder;

        encoder.interleave_mode(interleave_mode::none);
        encoder.interleave_mode(interleave_mode::line);
        encoder.interleave_mode(interleave_mode::sample);
    }

    TEST_METHOD(interleave_mode_bad)
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_interleave_mode,
            [&] { encoder.interleave_mode(static_cast<charls::interleave_mode>(-1)); });
        assert_expect_exception(jpegls_errc::invalid_argument_interleave_mode,
            [&] { encoder.interleave_mode(static_cast<charls::interleave_mode>(3)); });
    }

    TEST_METHOD(near_lossless)
    {
        jpegls_encoder encoder;

        encoder.near_lossless(0); // set lowest value.
        encoder.near_lossless(255); // set highest value.
    }

    TEST_METHOD(near_lossless_bad)
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_near_lossless, [&] { encoder.near_lossless(-1); });
        assert_expect_exception(jpegls_errc::invalid_argument_near_lossless, [&] { encoder.near_lossless(256); });
    }

    TEST_METHOD(estimated_destination_size_minimal_frame_info)
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1}); // minimum.
        const auto size = encoder.estimated_destination_size();
        Assert::IsTrue(size >= 1024);
    }

    TEST_METHOD(estimated_destination_size_16_bit)
    {
        jpegls_encoder encoder;

        encoder.frame_info({100, 100, 16, 1}); // minimum.
        const auto size = encoder.estimated_destination_size();
        Assert::IsTrue(size >= 100 * 100 * 2);
    }

    TEST_METHOD(estimated_destination_size_too_soon)
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_operation, [&] { encoder.estimated_destination_size(); });
    }

    TEST_METHOD(destination)
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(200);
        encoder.destination(destination);
    }

    TEST_METHOD(destination_can_only_be_set_once)
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(200);
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_operation, [&] {encoder.destination(destination); });
    }

    TEST_METHOD(write_standard_spiff_header)
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        Assert::AreEqual(static_cast<size_t>(34), encoder.bytes_written());
    }

    TEST_METHOD(write_standard_spiff_header_without_destination)
    {
        jpegls_encoder encoder;

        encoder.frame_info({ 1, 1, 2, 1 });

        assert_expect_exception(jpegls_errc::invalid_operation, [&] { encoder.write_standard_spiff_header(spiff_color_space::cmyk); });
    }

    TEST_METHOD(write_standard_spiff_header_without_frame_info)
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(100);
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_operation, [&] { encoder.write_standard_spiff_header(spiff_color_space::cmyk); });
    }

    TEST_METHOD(write_standard_spiff_header_twice)
    {
        jpegls_encoder encoder;

        encoder.frame_info({ 1, 1, 2, 1 });

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        assert_expect_exception(jpegls_errc::invalid_operation, [&] { encoder.write_standard_spiff_header(spiff_color_space::cmyk); });
    }

    TEST_METHOD(write_spiff_header)
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        spiff_header spiff_header{};
        spiff_header.width = 1;
        spiff_header.height = 1;
        encoder.write_spiff_header(spiff_header);

        Assert::AreEqual(static_cast<size_t>(34), encoder.bytes_written());
    }

    TEST_METHOD(write_spiff_header_invalid_height)
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

    TEST_METHOD(write_spiff_header_invalid_width)
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

    TEST_METHOD(write_spiff_entry)
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        encoder.write_spiff_entry(spiff_entry_tag::image_title, "test", 4);

        Assert::AreEqual(static_cast<size_t>(46), encoder.bytes_written());
    }

    TEST_METHOD(write_spiff_entry_twice)
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        encoder.write_spiff_entry(spiff_entry_tag::image_title, "test", 4);
        encoder.write_spiff_entry(spiff_entry_tag::image_title, "test", 4);

        Assert::AreEqual(static_cast<size_t>(58), encoder.bytes_written());
    }

    TEST_METHOD(write_empty_spiff_entry)
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        encoder.write_spiff_entry(spiff_entry_tag::image_title, nullptr, 0);

        Assert::AreEqual(static_cast<size_t>(42), encoder.bytes_written());
    }

    TEST_METHOD(write_spiff_entry_with_invalid_tag)
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        assert_expect_exception(jpegls_errc::invalid_argument, [&] { encoder.write_spiff_entry(1, "test", 4); });
    }

    TEST_METHOD(write_spiff_entry_with_invalid_size)
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.write_standard_spiff_header(spiff_color_space::cmyk);

        assert_expect_exception(jpegls_errc::invalid_argument_spiff_entry_size,
            [&] { encoder.write_spiff_entry(spiff_entry_tag::image_title, "test", 65528 + 1); });
    }

    TEST_METHOD(write_spiff_entry_without_spiff_header)
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] { encoder.write_spiff_entry(spiff_entry_tag::image_title, "test", 65528); });
    }

    TEST_METHOD(set_preset_coding_parameters)
    {
        jpegls_encoder encoder;

        charls_jpegls_pc_parameters pc_parameters{};
        encoder.preset_coding_parameters(pc_parameters);

        // No explicit test possible, code should remain stable.
        Assert::IsTrue(true);
    }

    TEST_METHOD(set_preset_coding_parameters_bad_values)
    {
        jpegls_encoder encoder;

        charls_jpegls_pc_parameters pc_parameters{1,1,1,1,1};

        assert_expect_exception(jpegls_errc::invalid_argument_pc_parameters,
            [&] { encoder.preset_coding_parameters(pc_parameters); });
    }

    TEST_METHOD(set_color_transformation_bad_value)
    {
        jpegls_encoder encoder;

        assert_expect_exception(jpegls_errc::invalid_argument_color_transformation,
            [&] { encoder.color_transformation(static_cast<color_transformation>(100)); });
    }

    TEST_METHOD(encode_without_destination)
    {
        jpegls_encoder encoder;

        encoder.frame_info({1, 1, 2, 1});
        vector<uint8_t> source(20);
        assert_expect_exception(jpegls_errc::invalid_operation,[&] { encoder.encode(source); });
    }

    TEST_METHOD(encode_without_frame_info)
    {
        jpegls_encoder encoder;

        vector<uint8_t> destination(20);
        encoder.destination(destination);
        vector<uint8_t> source(20);
        assert_expect_exception(jpegls_errc::invalid_operation,[&] { encoder.encode(source); });
    }

    TEST_METHOD(encode_with_spiff_header)
    {
        const array<uint8_t, 5> source{0, 1, 2, 3, 4};

        jpegls_encoder encoder;
        encoder.frame_info({5, 1, 8, 1});
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        encoder.write_standard_spiff_header(spiff_color_space::grayscale);
        encoder.encode(source);

        // TODO: add generic compare test function.
    }

    TEST_METHOD(encode_with_color_space)
    {
        const array<uint8_t, 6> source{0, 1, 2, 3, 4, 5};

        jpegls_encoder encoder;
        encoder.frame_info({2, 1, 8, 3});
        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination)
               .color_transformation(color_transformation::hp1);

        encoder.encode(source);

        // TODO: add generic compare test function.
    }

    TEST_METHOD(encode_16bit)
    {
        const array<uint8_t, 6> source{0, 1, 2, 3, 4, 5};

        jpegls_encoder encoder;
        encoder.frame_info({3, 1, 16, 1});

        vector<uint8_t> destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        encoder.encode(source);

        // TODO: add generic compare test function.
    }
};

} // namespace CharLSUnitTest
