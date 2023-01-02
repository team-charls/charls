// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include <array>
#include <tuple>
#include <vector>

#include "../src/jpeg_marker_code.h"
#include "../src/jpegls_preset_parameters_type.h"

#include "jpeg_test_stream_writer.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;
using std::error_code;
using std::ignore;
using std::numeric_limits;
using std::tie;
using std::vector;
using namespace charls_test;

namespace charls { namespace test {

namespace {

jpegls_decoder create_decoder(const vector<uint8_t>& source)
{
    return {source, true};
}

} // namespace


TEST_CLASS(jpegls_decoder_test)
{
public:
    TEST_METHOD(create_destroy) // NOLINT
    {
        // ReSharper disable once CppLocalVariableWithNonTrivialDtorIsNeverUsed
        jpegls_decoder decoder;
    }

    TEST_METHOD(create_and_move) // NOLINT
    {
        jpegls_decoder decoder1;

        jpegls_decoder decoder2(std::move(decoder1));

        jpegls_decoder decoder3;
        constexpr array<uint8_t, 10> buffer{};
        decoder3.source(buffer.data(), buffer.size());
        decoder3 = std::move(decoder2);

        jpegls_decoder decoder4(buffer.data(), buffer.size(), false);
        Assert::AreEqual(decoder4.frame_info().bits_per_sample, 0);
    }

    TEST_METHOD(set_source_twice_throws) // NOLINT
    {
        jpegls_decoder decoder;

        const vector<uint8_t> source(2000);
        decoder.source(source);
        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder, &source] { decoder.source(source); });
    }

    TEST_METHOD(read_spiff_header_without_source_throws) // NOLINT
    {
        jpegls_decoder decoder;

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { decoder.read_spiff_header(); });
    }

    TEST_METHOD(destination_size_without_reading_header_throws) // NOLINT
    {
        const jpegls_decoder decoder;

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.destination_size(); });
    }

    TEST_METHOD(read_header_without_source_throws) // NOLINT
    {
        jpegls_decoder decoder;

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { decoder.read_header(); });
    }

    TEST_METHOD(read_header_from_non_jpegls_data) // NOLINT
    {
        const vector<uint8_t> source(100);
        jpegls_decoder decoder{source, false};

        error_code ec;
        decoder.read_header(ec);

        Assert::IsTrue(ec == jpegls_errc::jpeg_marker_start_byte_not_found);
    }

    TEST_METHOD(frame_info_without_read_header) // NOLINT
    {
        const vector<uint8_t> source(2000);
        const jpegls_decoder decoder{source, false};

        Assert::AreEqual(0, decoder.frame_info().bits_per_sample);
        Assert::AreEqual(0, decoder.frame_info().component_count);
        Assert::AreEqual(0U, decoder.frame_info().height);
        Assert::AreEqual(0U, decoder.frame_info().width);
    }

    TEST_METHOD(frame_info_from_temporary_object) // NOLINT
    {
        const frame_info info{create_decoder(read_file("DataFiles/t8c0e0.jls")).frame_info()};

        Assert::AreEqual(8, info.bits_per_sample);
        Assert::AreEqual(3, info.component_count);
        Assert::AreEqual(256U, info.height);
        Assert::AreEqual(256U, info.width);
    }

    TEST_METHOD(interleave_mode_without_read_header_throws) // NOLINT
    {
        const vector<uint8_t> source(2000);
        const jpegls_decoder decoder{source, false};

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.interleave_mode(); });
    }

    TEST_METHOD(near_lossless_without_read_header_throws) // NOLINT
    {
        const vector<uint8_t> source(2000);
        const jpegls_decoder decoder{source, false};

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.near_lossless(); });
    }

    TEST_METHOD(preset_coding_parameters_without_read_header_throws) // NOLINT
    {
        jpegls_decoder decoder;

        const vector<uint8_t> source(2000);
        decoder.source(source);

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.preset_coding_parameters(); });
    }

    TEST_METHOD(destination_size) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder{source, true};

        constexpr size_t expected_destination_size{size_t{256} * 256 * 3};
        Assert::AreEqual(expected_destination_size, decoder.destination_size());
    }

    TEST_METHOD(destination_size_stride_interleave_none) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr size_t stride{512};
        constexpr size_t minimum_stride{256};
        constexpr size_t expected_destination_size{stride * 256 * 3 - (stride - minimum_stride)};
        Assert::AreEqual(expected_destination_size, decoder.destination_size(stride));
    }

    TEST_METHOD(destination_size_stride_interleave_none_16_bit) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t16e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr size_t stride{513};
        constexpr size_t minimum_stride{512};
        constexpr size_t expected_destination_size{stride * 256 - (stride - minimum_stride)};
        Assert::AreEqual(expected_destination_size, decoder.destination_size(stride));
    }

    TEST_METHOD(destination_size_stride_interleave_line) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c1e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr size_t stride{1024};
        constexpr size_t minimum_stride{size_t{3} * 256};
        constexpr size_t expected_destination_size{stride * 256 - (stride - minimum_stride)};
        Assert::AreEqual(expected_destination_size, decoder.destination_size(stride));
    }

    TEST_METHOD(destination_size_stride_interleave_sample) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c2e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr size_t stride{1024};
        constexpr size_t minimum_stride{size_t{3} * 256};
        constexpr size_t expected_destination_size{stride * 256 - (stride - minimum_stride)};
        Assert::AreEqual(expected_destination_size, decoder.destination_size(stride));
    }

    TEST_METHOD(destination_size_for_interleave_none_with_bad_stride_throws) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr uint32_t correct_stride{256};
        assert_expect_exception(jpegls_errc::invalid_argument_stride,
                                [&decoder, &correct_stride] { std::ignore = decoder.destination_size(correct_stride - 1); });
    }

    TEST_METHOD(destination_size_for_interleave_none_16_bit_with_bad_stride_throws) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t16e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr uint32_t correct_stride{256 * 2};
        assert_expect_exception(jpegls_errc::invalid_argument_stride,
                                [&decoder, &correct_stride] { std::ignore = decoder.destination_size(correct_stride - 1); });
    }

    TEST_METHOD(destination_size_for_sample_interleave_with_bad_stride_throws) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c2e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr uint32_t correct_stride{3 * 256};
        assert_expect_exception(jpegls_errc::invalid_argument_stride,
                                [&decoder, &correct_stride] { std::ignore = decoder.destination_size(correct_stride - 1); });
    }

    TEST_METHOD(destination_size_for_small_image_with_custom_stride) // NOLINT
    {
        const vector<uint8_t> source{read_file("8bit-monochrome-2x2.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr uint32_t stride{4};
        const size_t destination_size{decoder.destination_size(stride)};
        Assert::AreEqual(size_t{6}, destination_size);

        vector<uint8_t> destination(destination_size);
        decoder.decode(destination, stride);
    }

    TEST_METHOD(decode_reference_file_from_buffer) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};
        const jpegls_decoder decoder{source, true};

        vector<uint8_t> destination(decoder.destination_size());
        decoder.decode(destination);

        portable_anymap_file reference_file{
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.interleave_mode(), decoder.frame_info())};

        const auto& reference_image_data{reference_file.image_data()};
        for (size_t i{}; i != destination.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination[i]);
        }
    }

    TEST_METHOD(decode_with_default_pc_parameters_before_each_sos) // NOLINT
    {
        vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};
        insert_pc_parameters_segments(source, 3);

        const jpegls_decoder decoder{source, true};

        vector<uint8_t> destination(decoder.destination_size());
        decoder.decode(destination);

        portable_anymap_file reference_file{
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.interleave_mode(), decoder.frame_info())};

        const auto& reference_image_data{reference_file.image_data()};
        for (size_t i{}; i != destination.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination[i]);
        }
    }

    TEST_METHOD(start_of_scan_with_mixed_interleave_mode_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(1, 1, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);
        writer.write_byte(0x80);
        writer.write_start_of_scan_segment(1, 2, 0, interleave_mode::sample);

        const jpegls_decoder decoder(writer.buffer, true);
        std::vector<uint8_t> destination(decoder.destination_size());

        assert_expect_exception(jpegls_errc::parameter_value_not_supported,
                                [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(decode_with_destination_as_return) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};
        const jpegls_decoder decoder{source, true};
        const auto destination{decoder.decode<vector<uint8_t>>()};

        portable_anymap_file reference_file{
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.interleave_mode(), decoder.frame_info())};

        const auto& reference_image_data{reference_file.image_data()};
        for (size_t i{}; i != destination.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination[i]);
        }
    }

    TEST_METHOD(decode_with_16_bit_destination_as_return) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};
        const jpegls_decoder decoder{source, true};
        const auto destination{decoder.decode<vector<uint16_t>>()};

        portable_anymap_file reference_file{
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.interleave_mode(), decoder.frame_info())};

        const auto& reference_image_data{reference_file.image_data()};
        const auto* destination_as_bytes{reinterpret_cast<const uint8_t*>(destination.data())};
        for (size_t i{}; i != reference_image_data.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination_as_bytes[i]);
        }
    }

    TEST_METHOD(decode_without_reading_header_throws) // NOLINT
    {
        const jpegls_decoder decoder;

        vector<uint8_t> buffer(1000);
        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder, &buffer] { decoder.decode(buffer); });
    }

    TEST_METHOD(decode_color_interleave_none_with_too_small_buffer_throws) // NOLINT
    {
        decode_image_with_too_small_buffer_throws("DataFiles/t8c0e0.jls");
    }

    TEST_METHOD(decode_color_interleave_sample_with_too_small_buffer_throws) // NOLINT
    {
        decode_image_with_too_small_buffer_throws("DataFiles/t8c2e0.jls");
    }

    TEST_METHOD(decode_color_interleave_none_custom_stride_with_too_small_buffer_throws) // NOLINT
    {
        decode_image_with_too_small_buffer_throws("DataFiles/t8c0e0.jls", 256 + 1);
    }

    TEST_METHOD(decode_color_interleave_sample_custom_stride_with_too_small_buffer_throws) // NOLINT
    {
        decode_image_with_too_small_buffer_throws("DataFiles/t8c2e0.jls", 256 * 3 + 1);
    }

    TEST_METHOD(decode_color_interleave_none_with_too_small_stride_throws) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder{source, true};
        vector<uint8_t> destination(decoder.destination_size());

        constexpr uint32_t correct_stride{256};
        assert_expect_exception(jpegls_errc::invalid_argument_stride, [&decoder, &destination, &correct_stride] {
            decoder.decode(destination, correct_stride - 1);
        });
    }

    TEST_METHOD(decode_color_interleave_sample_with_too_small_stride_throws) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c2e0.jls")};

        const jpegls_decoder decoder{source, true};
        vector<uint8_t> destination(decoder.destination_size());

        constexpr uint32_t correct_stride{256 * 3};
        assert_expect_exception(jpegls_errc::invalid_argument_stride, [&decoder, &destination, correct_stride] {
            decoder.decode(destination, correct_stride - 1);
        });
    }

    TEST_METHOD(decode_color_interleave_none_with_standard_stride_works) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder{source, true};
        vector<uint8_t> destination(decoder.destination_size());
        const uint32_t standard_stride{decoder.frame_info().width};
        decoder.decode(destination, standard_stride);

        verify_decoded_bytes(decoder.interleave_mode(), decoder.frame_info(), destination, standard_stride,
                             "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_color_interleave_sample_with_standard_stride_works) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c2e0.jls")};

        const jpegls_decoder decoder{source, true};
        vector<uint8_t> destination(decoder.destination_size());
        const uint32_t standard_stride{decoder.frame_info().width * 3};
        decoder.decode(destination, standard_stride);

        verify_decoded_bytes(decoder.interleave_mode(), decoder.frame_info(), destination, standard_stride,
                             "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_color_interleave_none_with_custom_stride_works) // NOLINT
    {
        constexpr uint32_t custom_stride{256 + 1};
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder{source, true};
        vector<uint8_t> destination(decoder.destination_size(custom_stride));
        decoder.decode(destination, custom_stride);

        verify_decoded_bytes(decoder.interleave_mode(), decoder.frame_info(), destination, custom_stride,
                             "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_color_interleave_sample_with_custom_stride_works) // NOLINT
    {
        constexpr uint32_t custom_stride{256 * 3 + 1};
        const vector<uint8_t> source{read_file("DataFiles/t8c2e0.jls")};

        const jpegls_decoder decoder{source, true};
        vector<uint8_t> destination(decoder.destination_size(custom_stride));
        decoder.decode(destination, custom_stride);

        verify_decoded_bytes(decoder.interleave_mode(), decoder.frame_info(), destination, custom_stride,
                             "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_reference_to_mapping_table_selector_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;

        writer.write_start_of_image();
        writer.write_start_of_frame_segment(10, 10, 8, 3);
        writer.mapping_table_selector = 1;
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder{writer.buffer, false};

        assert_expect_exception(jpegls_errc::parameter_value_not_supported, [&decoder] { decoder.read_header(); });
    }

    TEST_METHOD(read_spiff_header) // NOLINT
    {
        const vector<uint8_t> source{create_test_spiff_header()};
        const jpegls_decoder decoder{source, true};

        Assert::IsTrue(decoder.spiff_header_has_value());

        const auto& header{decoder.spiff_header()};
        Assert::AreEqual(static_cast<int32_t>(spiff_profile_id::none), static_cast<int32_t>(header.profile_id));
        Assert::AreEqual(3, header.component_count);
        Assert::AreEqual(800U, header.height);
        Assert::AreEqual(600U, header.width);
        Assert::AreEqual(static_cast<int32_t>(spiff_color_space::rgb), static_cast<int32_t>(header.color_space));
        Assert::AreEqual(8, header.bits_per_sample);
        Assert::AreEqual(static_cast<int32_t>(spiff_compression_type::jpeg_ls),
                         static_cast<int32_t>(header.compression_type));
        Assert::AreEqual(static_cast<int32_t>(spiff_resolution_units::dots_per_inch),
                         static_cast<int32_t>(header.resolution_units));
        Assert::AreEqual(96U, header.vertical_resolution);
        Assert::AreEqual(1024U, header.horizontal_resolution);
    }

    TEST_METHOD(read_spiff_header_from_temporary_object) // NOLINT
    {
        const spiff_header header{create_decoder(create_test_spiff_header()).spiff_header()};

        Assert::AreEqual(static_cast<int32_t>(spiff_profile_id::none), static_cast<int32_t>(header.profile_id));
        Assert::AreEqual(3, header.component_count);
        Assert::AreEqual(800U, header.height);
        Assert::AreEqual(600U, header.width);
        Assert::AreEqual(static_cast<int32_t>(spiff_color_space::rgb), static_cast<int32_t>(header.color_space));
        Assert::AreEqual(8, header.bits_per_sample);
        Assert::AreEqual(static_cast<int32_t>(spiff_compression_type::jpeg_ls),
                         static_cast<int32_t>(header.compression_type));
        Assert::AreEqual(static_cast<int32_t>(spiff_resolution_units::dots_per_inch),
                         static_cast<int32_t>(header.resolution_units));
        Assert::AreEqual(96U, header.vertical_resolution);
        Assert::AreEqual(1024U, header.horizontal_resolution);
    }

    TEST_METHOD(read_spiff_header_from_non_jpegls_data) // NOLINT
    {
        const vector<uint8_t> source(100);
        jpegls_decoder decoder{source, false};

        error_code ec;
        ignore = decoder.read_spiff_header(ec);

        Assert::IsTrue(ec == jpegls_errc::jpeg_marker_start_byte_not_found);
    }

    TEST_METHOD(read_spiff_header_from_jpegls_without_spiff) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder{source, true};

        Assert::IsFalse(decoder.spiff_header_has_value());

        const frame_info& frame_info{decoder.frame_info()};

        Assert::AreEqual(3, frame_info.component_count);
        Assert::AreEqual(8, frame_info.bits_per_sample);
        Assert::AreEqual(256U, frame_info.height);
        Assert::AreEqual(256U, frame_info.width);
    }

    TEST_METHOD(read_invalid_spiff_header_with_read_header) // NOLINT
    {
        const vector<uint8_t> source{create_test_spiff_header(2, 0, true, 1)};
        jpegls_decoder decoder{source, false};

        decoder.read_spiff_header();
        std::error_code ec;
        decoder.read_header(ec);

        Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_spiff_header), ec.value());
    }

    TEST_METHOD(read_invalid_spiff_header_throws) // NOLINT
    {
        const vector<uint8_t> source{create_test_spiff_header(2, 0, true, 1)};

        assert_expect_exception(jpegls_errc::invalid_spiff_header, [&source] {
            // ReSharper disable once CppLocalVariableWithNonTrivialDtorIsNeverUsed
            jpegls_decoder decoder{source, true};
        });
    }

    TEST_METHOD(read_header_twice_throws) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        jpegls_decoder decoder{source, true};

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.read_header(); });
    }

    TEST_METHOD(decode_twice_throws) // NOLINT
    {
        constexpr frame_info frame_info{512, 512, 8, 1};
        const vector<uint8_t> source_to_encode(static_cast<size_t>(frame_info.width) * frame_info.height);

        const auto encoded{jpegls_encoder::encode(source_to_encode, frame_info)};

        const jpegls_decoder decoder{encoded, true};
        vector<uint8_t> destination(decoder.destination_size());
        decoder.decode(destination);

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(simple_decode) // NOLINT
    {
        const vector<uint8_t> encoded_source{read_file("DataFiles/t8c0e0.jls")};

        vector<uint8_t> decoded_destination;
        frame_info frame_info;
        interleave_mode interleave_mode;
        tie(frame_info, interleave_mode) = jpegls_decoder::decode(encoded_source, decoded_destination);

        Assert::AreEqual(3, frame_info.component_count);
        Assert::AreEqual(8, frame_info.bits_per_sample);
        Assert::AreEqual(256U, frame_info.height);
        Assert::AreEqual(256U, frame_info.width);
        Assert::AreEqual(interleave_mode::none, interleave_mode);

        const size_t expected_size = static_cast<size_t>(frame_info.height) * frame_info.width * frame_info.component_count;
        Assert::AreEqual(expected_size, decoded_destination.size());
    }

    TEST_METHOD(simple_decode_to_uint16_buffer) // NOLINT
    {
        const vector<uint8_t> encoded_source{read_file("DataFiles/t8c0e0.jls")};

        vector<uint16_t> decoded_destination;
        frame_info frame_info;
        interleave_mode interleave_mode;
        tie(frame_info, interleave_mode) = jpegls_decoder::decode(encoded_source, decoded_destination);

        Assert::AreEqual(3, frame_info.component_count);
        Assert::AreEqual(8, frame_info.bits_per_sample);
        Assert::AreEqual(256U, frame_info.height);
        Assert::AreEqual(256U, frame_info.width);
        Assert::AreEqual(interleave_mode::none, interleave_mode);

        const size_t expected_size{static_cast<size_t>(frame_info.height) * frame_info.width * frame_info.component_count};
        Assert::AreEqual(expected_size, decoded_destination.size() * sizeof(uint16_t));
    }

    TEST_METHOD(decode_file_with_ff_in_entropy_data_throws) // NOLINT
    {
        const vector<uint8_t> source{read_file("ff_in_entropy_data.jls")};

        const jpegls_decoder decoder{source, true};

        const auto& frame_info{decoder.frame_info()};
        Assert::AreEqual(1, frame_info.component_count);
        Assert::AreEqual(12, frame_info.bits_per_sample);
        Assert::AreEqual(1216U, frame_info.height);
        Assert::AreEqual(968U, frame_info.width);

        vector<uint8_t> destination(decoder.destination_size());

        assert_expect_exception(jpegls_errc::invalid_encoded_data,
                                [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(decode_with_missing_end_of_image_marker_throws) // NOLINT
    {
        constexpr frame_info frame_info{512, 512, 8, 1};
        const vector<uint8_t> source_to_encode(static_cast<size_t>(frame_info.width) * frame_info.height);

        const auto encoded{jpegls_encoder::encode(source_to_encode, frame_info)};

        {
            // Copy the vector to ensure source buffer has a defined limit (resize() keeps memory)
            // that can be checked with address sanitizer.
            const vector<uint8_t> source(encoded.cbegin(), encoded.cend() - 1);

            const jpegls_decoder decoder{source, true};
            vector<uint8_t> destination(decoder.destination_size());
            assert_expect_exception(jpegls_errc::source_buffer_too_small,
                                    [&decoder, &destination] { decoder.decode(destination); });
        }

        {
            const vector<uint8_t> source(encoded.cbegin(), encoded.cend() - 2);
            const jpegls_decoder decoder{source, true};
            vector<uint8_t> destination(decoder.destination_size());

            assert_expect_exception(jpegls_errc::source_buffer_too_small,
                                    [&decoder, &destination] { decoder.decode(destination); });
        }

        {
            vector<uint8_t> source(encoded);
            source[source.size() - 1] = 0x33;
            const jpegls_decoder decoder{source, true};
            vector<uint8_t> destination(decoder.destination_size());

            assert_expect_exception(jpegls_errc::end_of_image_marker_not_found,
                                    [&decoder, &destination] { decoder.decode(destination); });
        }
    }

    TEST_METHOD(decode_file_with_golomb_large_then_k_max_throws) // NOLINT
    {
        const vector<uint8_t> source{read_file("fuzzy_input_golomb_16.jls")};

        const jpegls_decoder decoder{source, true};

        const auto& frame_info{decoder.frame_info()};
        Assert::AreEqual(3, frame_info.component_count);
        Assert::AreEqual(16, frame_info.bits_per_sample);
        Assert::AreEqual(65516U, frame_info.height);
        Assert::AreEqual(1U, frame_info.width);

        vector<uint8_t> destination(decoder.destination_size());

        assert_expect_exception(jpegls_errc::invalid_encoded_data,
                                [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(decode_file_with_missing_restart_marker_throws) // NOLINT
    {
        vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        // Insert a DRI marker segment to trigger that restart markers are used.
        jpeg_test_stream_writer stream_writer;
        stream_writer.write_define_restart_interval(10, 3);
        const auto it{source.begin() + 2};
        source.insert(it, stream_writer.buffer.cbegin(), stream_writer.buffer.cend());

        const jpegls_decoder decoder{source, true};
        vector<uint8_t> destination(decoder.destination_size());

        assert_expect_exception(jpegls_errc::restart_marker_not_found,
                                [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(decode_file_with_incorrect_restart_marker_throws) // NOLINT
    {
        vector<uint8_t> source{read_file("DataFiles/test8_ilv_none_rm_7.jls")};

        // Change the first restart marker to the second.
        auto it{find_scan_header(source.begin(), source.end())};
        it = find_first_restart_marker(it + 1, source.end());
        ++it;
        *it = 0xD1;

        const jpegls_decoder decoder{source, true};
        vector<uint8_t> destination(decoder.destination_size());

        assert_expect_exception(jpegls_errc::restart_marker_not_found,
                                [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(decode_file_with_extra_begin_bytes_for_restart_marker_code) // NOLINT
    {
        vector<uint8_t> source{read_file("DataFiles/test8_ilv_none_rm_7.jls")};

        // Add additional 0xFF marker begin bytes
        auto it{find_scan_header(source.begin(), source.end())};
        it = find_first_restart_marker(it + 1, source.end());
        const array<uint8_t, 7> extra_begin_bytes{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        source.insert(it, extra_begin_bytes.cbegin(), extra_begin_bytes.cend());

        const jpegls_decoder decoder{source, true};
        portable_anymap_file reference_file{
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.interleave_mode(), decoder.frame_info())};

        test_compliance(source, reference_file.image_data(), false);
    }

    TEST_METHOD(decode_file_that_ends_after_restart_marker_throws) // NOLINT
    {
        vector<uint8_t> source{read_file("DataFiles/test8_ilv_none_rm_7.jls")};

        auto it{find_scan_header(source.begin(), source.end())};
        it = find_first_restart_marker(it + 1, source.end());

        // Copy the vector to ensure source buffer has a defined limit (resize() keeps memory)
        // that can be checked with address sanitizer.
        const vector<uint8_t> too_small_source(source.begin(), it);

        const jpegls_decoder decoder{too_small_source, true};
        vector<uint8_t> destination(decoder.destination_size());

        assert_expect_exception(jpegls_errc::source_buffer_too_small,
                                [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(read_comment) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_segment(jpeg_marker_code::comment, "hello", 5);
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());

        const void* actual_data{};
        size_t actual_size{};
        decoder.at_comment([&actual_data, &actual_size](const void* data, const size_t size) noexcept {
            actual_data = data;
            actual_size = size;
        });

        decoder.read_header();

        Assert::AreEqual(size_t{5}, actual_size);
        Assert::IsTrue(memcmp("hello", actual_data, actual_size) == 0);
    }

    TEST_METHOD(read_comment_while_already_unregisted) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_segment(jpeg_marker_code::comment, "hello", 5);
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());

        bool callback_called{};
        decoder.at_comment([&callback_called](const void*, const size_t) noexcept { callback_called = true; })
            .at_comment(nullptr);

        decoder.read_header();

        Assert::IsFalse(callback_called);
    }

    TEST_METHOD(at_comment_that_throws_returns_callback_error) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_segment(jpeg_marker_code::comment, "hello", 5);
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());

        decoder.at_comment([](const void*, const size_t) { throw std::runtime_error("something failed"); });

        assert_expect_exception(jpegls_errc::callback_failed, [&decoder] { decoder.read_header(); });
    }

    TEST_METHOD(read_application_data) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_segment(jpeg_marker_code::application_data0, "hello", 5);
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());

        int32_t actual_application_data_id{1};
        const void* actual_data{};
        size_t actual_size{};
        decoder.at_application_data([&actual_application_data_id, &actual_data, &actual_size](
                                        const int32_t application_data_id, const void* data, const size_t size) noexcept {
            actual_application_data_id = application_data_id;
            actual_data = data;
            actual_size = size;
        });

        decoder.read_header();

        Assert::AreEqual(0, actual_application_data_id);
        Assert::AreEqual(size_t{5}, actual_size);
        Assert::IsTrue(memcmp("hello", actual_data, actual_size) == 0);
    }

    TEST_METHOD(read_application_data_while_already_unregisted) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_segment(jpeg_marker_code::application_data0, "hello", 5);
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());

        bool callback_called{};
        decoder
            .at_application_data([&callback_called](int32_t, const void*, const size_t) noexcept { callback_called = true; })
            .at_application_data(nullptr);

        decoder.read_header();

        Assert::IsFalse(callback_called);
    }

    TEST_METHOD(at_application_data_that_throws_returns_callback_error) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_segment(jpeg_marker_code::application_data0, "hello", 5);
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());

        decoder.at_application_data(
            [](int32_t, const void*, const size_t) { throw std::runtime_error("something failed"); });

        assert_expect_exception(jpegls_errc::callback_failed, [&decoder] { decoder.read_header(); });
    }

    TEST_METHOD(oversize_image_dimension_before_start_of_frame) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        constexpr uint32_t height{numeric_limits<uint16_t>::max() + 1U};
        constexpr uint32_t width{99};
        writer.write_oversize_image_dimension(3, height, width);
        writer.write_start_of_frame_segment(0, 0, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());
        decoder.read_header();

        Assert::AreEqual(height, decoder.frame_info().height);
        Assert::AreEqual(width, decoder.frame_info().width);
    }

    TEST_METHOD(oversize_image_dimension_zero_before_start_of_frame) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        constexpr uint32_t height{numeric_limits<uint16_t>::max()};
        constexpr uint32_t width{99};
        writer.write_oversize_image_dimension(2, 0, 0);
        writer.write_start_of_frame_segment(width, height, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());
        decoder.read_header();

        Assert::AreEqual(height, decoder.frame_info().height);
        Assert::AreEqual(width, decoder.frame_info().width);
    }


    TEST_METHOD(oversize_image_dimension_with_invalid_number_of_bytes_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        constexpr auto invalid_number_of_bytes{1};
        writer.write_oversize_image_dimension(invalid_number_of_bytes, 1, 1);
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());

        assert_expect_exception(jpegls_errc::invalid_parameter_jpegls_preset_parameters,
                                [&decoder] { decoder.read_header(); });
    }

    TEST_METHOD(oversize_image_dimension_change_width_after_start_of_frame_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        constexpr uint32_t height{numeric_limits<uint16_t>::max()};
        constexpr uint32_t width{99};
        writer.write_start_of_frame_segment(width, height, 8, 3);
        writer.write_oversize_image_dimension(2, 0, 10);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());

        assert_expect_exception(jpegls_errc::invalid_parameter_width, [&decoder] { decoder.read_header(); });
    }

    TEST_METHOD(start_of_frame_changes_height_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        constexpr uint32_t height{numeric_limits<uint16_t>::max()};
        constexpr uint32_t width{0};
        writer.write_oversize_image_dimension(2, 10, width);
        writer.write_start_of_frame_segment(width, height, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());

        assert_expect_exception(jpegls_errc::invalid_parameter_height, [&decoder] { decoder.read_header(); });
    }

    TEST_METHOD(oversize_image_dimension_bad_segment_size_throws) // NOLINT
    {
        oversize_image_dimension_bad_segment_size_throws(2);
        oversize_image_dimension_bad_segment_size_throws(3);
        oversize_image_dimension_bad_segment_size_throws(4);
    }

    TEST_METHOD(oversize_image_dimension_that_causes_overflow_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        constexpr uint32_t height{numeric_limits<uint32_t>::max()};
        constexpr uint32_t width{numeric_limits<uint32_t>::max()};
        writer.write_oversize_image_dimension(4, height, width);
        constexpr size_t component_count{2};
        writer.write_start_of_frame_segment(0, 0, 8, component_count);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());
        decoder.read_header();

#if INTPTR_MAX == INT64_MAX
        Assert::AreEqual(component_count * numeric_limits<uint32_t>::max() * numeric_limits<uint32_t>::max(),
                         decoder.destination_size());
#elif INTPTR_MAX == INT32_MAX
        assert_expect_exception(jpegls_errc::parameter_value_not_supported,
                                [&decoder] { ignore = decoder.destination_size(); });
#else
#error Unknown pointer size or missing size macros!
#endif
    }

    TEST_METHOD(decode_to_buffer_with_uint16_size_works) // NOLINT
    {
        // These are compile time checks to detect issues with overloads that have similar conversions.
        constexpr frame_info frame_info{100, 100, 8, 1};
        const vector<uint8_t> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        const vector<uint8_t> encoded_source{
            jpegls_encoder::encode(source, frame_info, interleave_mode::none, encoding_options::even_destination_size)};

        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        vector<uint8_t> destination(decoder.destination_size());

        void* data{destination.data()};
        const uint16_t size{static_cast<uint16_t>(destination.size())};

        // size is not a perfect match and needs a conversion.
        decoder.decode(data, size);
    }

private:
    static vector<uint8_t>::iterator find_scan_header(const vector<uint8_t>::iterator begin,
                                                      const vector<uint8_t>::iterator end) noexcept
    {
        constexpr uint8_t start_of_scan{0xDA};

        for (auto it{begin}; it != end; ++it)
        {
            if (*it == 0xFF && it + 1 != end && *(it + 1) == start_of_scan)
                return it;
        }

        return end;
    }

    static vector<uint8_t>::iterator find_first_restart_marker(const vector<uint8_t>::iterator begin,
                                                               const vector<uint8_t>::iterator end) noexcept
    {
        constexpr uint8_t first_restart_marker{0xD0};

        for (auto it{begin}; it != end; ++it)
        {
            if (*it == 0xFF && it + 1 != end && *(it + 1) == first_restart_marker)
                return it;
        }

        return end;
    }

    static vector<uint8_t> create_default_pc_parameters_segment()
    {
        vector<uint8_t> segment;

        segment.push_back(uint8_t{0xFF});
        segment.push_back(static_cast<uint8_t>(jpeg_marker_code::jpegls_preset_parameters));
        push_back(segment, static_cast<uint16_t>(11 + sizeof(uint16_t)));
        segment.push_back(static_cast<uint8_t>(jpegls_preset_parameters_type::preset_coding_parameters));
        push_back(segment, uint16_t{0});
        push_back(segment, uint16_t{0});
        push_back(segment, uint16_t{0});
        push_back(segment, uint16_t{0});
        push_back(segment, uint16_t{0});

        return segment;
    }

    static void insert_pc_parameters_segments(vector<uint8_t> & jpegls_source, const int component_count)
    {
        const auto pcp_segment{create_default_pc_parameters_segment()};

        auto it{jpegls_source.begin()};
        for (int i{}; i != component_count; ++i)
        {
            it = find_scan_header(it, jpegls_source.end());
            it = jpegls_source.insert(it, pcp_segment.cbegin(), pcp_segment.cend());
            it += static_cast<vector<uint8_t>::difference_type>(pcp_segment.size() + 2U);
        }
    }

    static void oversize_image_dimension_bad_segment_size_throws(const uint32_t number_of_bytes)
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        constexpr uint32_t height{numeric_limits<uint16_t>::max()};
        constexpr uint32_t width{0};
        writer.write_oversize_image_dimension(number_of_bytes, 10, width, true);
        writer.write_start_of_frame_segment(width, height, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&decoder] { decoder.read_header(); });
    }

    static void decode_image_with_too_small_buffer_throws(const char* image_filename, const uint32_t stride = 0)
    {
        const vector<uint8_t> source{read_file(image_filename)};

        const jpegls_decoder decoder{source, true};
        vector<uint8_t> destination(decoder.destination_size(stride) - 1);

        assert_expect_exception(jpegls_errc::destination_buffer_too_small,
                                [&decoder, &destination, &stride] { decoder.decode(destination, stride); });
    }
};

}} // namespace charls::test
