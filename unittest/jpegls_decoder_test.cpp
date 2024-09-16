// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "util.hpp"

#include <charls/jpegls_decoder.hpp>
#include <charls/jpegls_encoder.hpp>

#include <array>
#include <tuple>
#include <vector>

#include "../src/jpeg_marker_code.hpp"
#include "../src/jpegls_preset_parameters_type.hpp"

#include "jpeg_test_stream_writer.hpp"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;
using std::byte;
using std::error_code;
using std::ignore;
using std::numeric_limits;
using std::vector;
using namespace charls_test;

namespace charls::test {

namespace {

[[nodiscard]]
jpegls_decoder create_decoder(const vector<byte>& source)
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
        constexpr array<byte, 10> buffer{};
        decoder3.source(buffer.data(), buffer.size());
        decoder3 = std::move(decoder2);

        jpegls_decoder decoder4(buffer.data(), buffer.size(), false);
        Assert::AreEqual(decoder4.frame_info().bits_per_sample, 0);
    }

    TEST_METHOD(set_source_twice_throws) // NOLINT
    {
        jpegls_decoder decoder;

        const vector<byte> source(2000);
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

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.get_destination_size(); });
    }

    TEST_METHOD(read_header_without_source_throws) // NOLINT
    {
        jpegls_decoder decoder;

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { decoder.read_header(); });
    }

    TEST_METHOD(read_header_from_non_jpegls_data) // NOLINT
    {
        const vector<byte> source(100);
        jpegls_decoder decoder{source, false};

        error_code ec;
        decoder.read_header(ec);

        Assert::IsTrue(ec == jpegls_errc::jpeg_marker_start_byte_not_found);
    }

    TEST_METHOD(frame_info_without_read_header) // NOLINT
    {
        const vector<byte> source(2000);
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
        const vector<byte> source(2000);
        const jpegls_decoder decoder{source, false};

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.get_interleave_mode(); });
    }

    TEST_METHOD(near_lossless_without_read_header_throws) // NOLINT
    {
        const vector<byte> source(2000);
        const jpegls_decoder decoder{source, false};

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.get_near_lossless(); });
    }

    TEST_METHOD(preset_coding_parameters_without_read_header_throws) // NOLINT
    {
        jpegls_decoder decoder;

        const vector<byte> source(2000);
        decoder.source(source);

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.preset_coding_parameters(); });
    }

    TEST_METHOD(get_destination_size) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder{source, true};

        constexpr size_t expected_destination_size{size_t{256} * 256 * 3};
        Assert::AreEqual(expected_destination_size, decoder.get_destination_size());
    }

    TEST_METHOD(get_destination_size_stride_interleave_none) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c0e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr size_t stride{512};
        constexpr size_t minimum_stride{256};
        constexpr size_t expected_destination_size{stride * 256 * 3 - (stride - minimum_stride)};
        Assert::AreEqual(expected_destination_size, decoder.get_destination_size(stride));
    }

    TEST_METHOD(get_destination_size_stride_interleave_none_16_bit) // NOLINT
    {
        const auto source{read_file("DataFiles/t16e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr size_t stride{513};
        constexpr size_t minimum_stride{512};
        constexpr size_t expected_destination_size{stride * 256 - (stride - minimum_stride)};
        Assert::AreEqual(expected_destination_size, decoder.get_destination_size(stride));
    }

    TEST_METHOD(get_destination_size_stride_interleave_line) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c1e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr size_t stride{1024};
        constexpr size_t minimum_stride{size_t{3} * 256};
        constexpr size_t expected_destination_size{stride * 256 - (stride - minimum_stride)};
        Assert::AreEqual(expected_destination_size, decoder.get_destination_size(stride));
    }

    TEST_METHOD(get_destination_size_stride_interleave_sample) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c2e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr size_t stride{1024};
        constexpr size_t minimum_stride{size_t{3} * 256};
        constexpr size_t expected_destination_size{stride * 256 - (stride - minimum_stride)};
        Assert::AreEqual(expected_destination_size, decoder.get_destination_size(stride));
    }

    TEST_METHOD(get_destination_size_for_interleave_none_with_bad_stride_throws) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c0e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr uint32_t correct_stride{256};
        assert_expect_exception(jpegls_errc::invalid_argument_stride,
                                [&decoder, &correct_stride] { ignore = decoder.get_destination_size(correct_stride - 1); });
    }

    TEST_METHOD(get_destination_size_for_interleave_none_16_bit_with_bad_stride_throws) // NOLINT
    {
        const auto source{read_file("DataFiles/t16e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr uint32_t correct_stride{256 * 2};
        assert_expect_exception(jpegls_errc::invalid_argument_stride,
                                [&decoder, &correct_stride] { ignore = decoder.get_destination_size(correct_stride - 1); });
    }

    TEST_METHOD(get_destination_size_for_sample_interleave_with_bad_stride_throws) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c2e0.jls")};
        const jpegls_decoder decoder{source, true};

        constexpr uint32_t correct_stride{3 * 256};
        assert_expect_exception(jpegls_errc::invalid_argument_stride,
                                [&decoder, &correct_stride] { ignore = decoder.get_destination_size(correct_stride - 1); });
    }

    TEST_METHOD(get_destination_size_for_small_image_with_custom_stride) // NOLINT
    {
        const auto source{read_file("DataFiles/8bit-monochrome-2x2.jls")};
        jpegls_decoder decoder{source, true};

        constexpr uint32_t stride{4};
        const size_t destination_size{decoder.get_destination_size(stride)};
        Assert::AreEqual(size_t{6}, destination_size);

        vector<byte> destination(destination_size);
        decoder.decode(destination, stride);
    }

    TEST_METHOD(decode_reference_file_from_buffer) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c0e0.jls")};
        jpegls_decoder decoder{source, true};

        vector<byte> destination(decoder.get_destination_size());
        decoder.decode(destination);

        portable_anymap_file reference_file{
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.get_interleave_mode(), decoder.frame_info())};

        const auto& reference_image_data{reference_file.image_data()};
        for (size_t i{}; i != destination.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination[i]);
        }
    }

    TEST_METHOD(decode_fuzzy_input_no_valid_bits_at_the_end_throws) // NOLINT
    {
        // Remark: exception is thrown from different location when lossless_traits becomes more generic.
        const auto source{read_file("DataFiles/fuzzy-input-no-valid-bits-at-the-end.jls")};
        jpegls_decoder decoder{source, true};

        vector<byte> destination(decoder.get_destination_size());

        assert_expect_exception(jpegls_errc::invalid_data, [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(decode_fuzzy_input_bad_run_mode_golomb_code_throws) // NOLINT
    {
        const auto source{read_file("DataFiles/fuzzy-input-bad-run-mode-golomb-code.jls")};
        jpegls_decoder decoder{source, true};

        vector<byte> destination(decoder.get_destination_size());

        assert_expect_exception(jpegls_errc::invalid_data, [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(get_destination_size_returns_zero_for_abbreviated_table_specification) // NOLINT
    {
        const vector<byte> table_data(4);
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_jpegls_preset_parameters_segment(1, 1, table_data, false);
        writer.write_marker(jpeg_marker_code::end_of_image);

        const jpegls_decoder decoder{writer.buffer, true};
        const auto size = decoder.get_destination_size();

        Assert::AreEqual(size_t{0}, size);
    }

    TEST_METHOD(decode_with_default_pc_parameters_before_each_sos) // NOLINT
    {
        auto source{read_file("DataFiles/t8c0e0.jls")};
        insert_pc_parameters_segments(source, 3);

        jpegls_decoder decoder{source, true};

        vector<byte> destination(decoder.get_destination_size());
        decoder.decode(destination);

        portable_anymap_file reference_file{
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.get_interleave_mode(), decoder.frame_info())};

        const auto& reference_image_data{reference_file.image_data()};
        for (size_t i{}; i != destination.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination[i]);
        }
    }

    TEST_METHOD(decode_with_destination_as_return) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c0e0.jls")};
        jpegls_decoder decoder{source, true};
        const auto destination{decoder.decode<vector<byte>>()};

        portable_anymap_file reference_file{
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.get_interleave_mode(), decoder.frame_info())};

        const auto& reference_image_data{reference_file.image_data()};
        for (size_t i{}; i != destination.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination[i]);
        }
    }

    TEST_METHOD(decode_with_16_bit_destination_as_return) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c0e0.jls")};
        jpegls_decoder decoder{source, true};
        const auto destination{decoder.decode<vector<uint16_t>>()};

        portable_anymap_file reference_file{
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.get_interleave_mode(), decoder.frame_info())};

        const auto& reference_image_data{reference_file.image_data()};
        const auto* destination_as_bytes{reinterpret_cast<const byte*>(destination.data())};
        for (size_t i{}; i != reference_image_data.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination_as_bytes[i]);
        }
    }

    TEST_METHOD(decode_without_reading_header_throws) // NOLINT
    {
        jpegls_decoder decoder;

        vector<byte> buffer(1000);
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
        decode_image_with_too_small_buffer_throws("DataFiles/t8c0e0.jls", 256 + 1, 1 + 1);
    }

    TEST_METHOD(decode_color_interleave_sample_custom_stride_with_too_small_buffer_throws) // NOLINT
    {
        decode_image_with_too_small_buffer_throws("DataFiles/t8c2e0.jls", 256 * 3 + 1, 1 + 1);
    }

    TEST_METHOD(decode_color_interleave_none_with_too_small_stride_throws) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c0e0.jls")};

        jpegls_decoder decoder{source, true};
        vector<byte> destination(decoder.get_destination_size());

        constexpr uint32_t correct_stride{256};
        assert_expect_exception(jpegls_errc::invalid_argument_stride, [&decoder, &destination, &correct_stride] {
            decoder.decode(destination, correct_stride - 1);
        });
    }

    TEST_METHOD(decode_color_interleave_sample_with_too_small_stride_throws) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c2e0.jls")};

        jpegls_decoder decoder{source, true};
        vector<byte> destination(decoder.get_destination_size());

        constexpr uint32_t correct_stride{256 * 3};
        assert_expect_exception(jpegls_errc::invalid_argument_stride, [&decoder, &destination, correct_stride] {
            decoder.decode(destination, correct_stride - 1);
        });
    }

    TEST_METHOD(decode_color_interleave_none_with_standard_stride_works) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c0e0.jls")};

        jpegls_decoder decoder{source, true};
        vector<byte> destination(decoder.get_destination_size());
        const uint32_t standard_stride{decoder.frame_info().width};
        decoder.decode(destination, standard_stride);

        verify_decoded_bytes(decoder.get_interleave_mode(), decoder.frame_info(), destination, standard_stride,
                             "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_color_interleave_sample_with_standard_stride_works) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c2e0.jls")};

        jpegls_decoder decoder{source, true};
        vector<byte> destination(decoder.get_destination_size());
        const uint32_t standard_stride{decoder.frame_info().width * 3};
        decoder.decode(destination, standard_stride);

        verify_decoded_bytes(decoder.get_interleave_mode(), decoder.frame_info(), destination, standard_stride,
                             "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_color_interleave_none_with_custom_stride_works) // NOLINT
    {
        constexpr uint32_t custom_stride{256 + 1};
        const auto source{read_file("DataFiles/t8c0e0.jls")};

        jpegls_decoder decoder{source, true};
        vector<byte> destination(decoder.get_destination_size(custom_stride));
        decoder.decode(destination, custom_stride);

        verify_decoded_bytes(decoder.get_interleave_mode(), decoder.frame_info(), destination, custom_stride,
                             "DataFiles/test8.ppm");
    }

    TEST_METHOD(decode_color_interleave_sample_with_custom_stride_works) // NOLINT
    {
        constexpr uint32_t custom_stride{256 * 3 + 1};
        const auto source{read_file("DataFiles/t8c2e0.jls")};

        jpegls_decoder decoder{source, true};
        vector<byte> destination(decoder.get_destination_size(custom_stride));
        decoder.decode(destination, custom_stride);

        verify_decoded_bytes(decoder.get_interleave_mode(), decoder.frame_info(), destination, custom_stride,
                             "DataFiles/test8.ppm");
    }

    TEST_METHOD(read_spiff_header) // NOLINT
    {
        const auto source{create_test_spiff_header()};
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

    TEST_METHOD(read_spiff_header_from_temporary_decoder) // NOLINT
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
        const vector<byte> source(100);
        jpegls_decoder decoder{source, false};

        error_code ec;
        ignore = decoder.read_spiff_header(ec);

        Assert::IsTrue(ec == jpegls_errc::jpeg_marker_start_byte_not_found);
    }

    TEST_METHOD(read_spiff_header_from_jpegls_without_spiff) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c0e0.jls")};
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
        const auto source{create_test_spiff_header(2, 0, true, 1)};
        jpegls_decoder decoder{source, false};

        decoder.read_spiff_header();
        std::error_code ec;
        decoder.read_header(ec);

        Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_spiff_header), ec.value());
    }

    TEST_METHOD(read_invalid_spiff_header_throws) // NOLINT
    {
        const auto source{create_test_spiff_header(2, 0, true, 1)};

        assert_expect_exception(jpegls_errc::invalid_spiff_header, [&source] {
            // ReSharper disable once CppLocalVariableWithNonTrivialDtorIsNeverUsed
            jpegls_decoder decoder{source, true};
        });
    }

    TEST_METHOD(read_header_twice_throws) // NOLINT
    {
        const auto source{read_file("DataFiles/t8c0e0.jls")};
        jpegls_decoder decoder{source, true};

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.read_header(); });
    }

    TEST_METHOD(decode_twice_throws) // NOLINT
    {
        constexpr frame_info frame_info{512, 512, 8, 1};
        const vector<byte> source_to_encode(static_cast<size_t>(frame_info.width) * frame_info.height);

        const auto encoded{jpegls_encoder::encode(source_to_encode, frame_info)};

        jpegls_decoder decoder{encoded, true};
        vector<byte> destination(decoder.get_destination_size());
        decoder.decode(destination);

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(simple_decode) // NOLINT
    {
        const auto encoded_source{read_file("DataFiles/t8c0e0.jls")};

        vector<byte> decoded_destination;
        const auto [frame_info, interleave_mode]{jpegls_decoder::decode(encoded_source, decoded_destination)};

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
        const auto encoded_source{read_file("DataFiles/t8c0e0.jls")};

        vector<uint16_t> decoded_destination;
        const auto [frame_info, interleave_mode]{jpegls_decoder::decode(encoded_source, decoded_destination)};

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
        const auto source{read_file("ff_in_entropy_data.jls")};

        jpegls_decoder decoder{source, true};

        const auto& frame_info{decoder.frame_info()};
        Assert::AreEqual(1, frame_info.component_count);
        Assert::AreEqual(12, frame_info.bits_per_sample);
        Assert::AreEqual(1216U, frame_info.height);
        Assert::AreEqual(968U, frame_info.width);

        vector<byte> destination(decoder.get_destination_size());

        assert_expect_exception(jpegls_errc::invalid_data,
                                [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(decode_with_missing_end_of_image_marker_throws) // NOLINT
    {
        constexpr frame_info frame_info{512, 512, 8, 1};
        const vector<byte> source_to_encode(static_cast<size_t>(frame_info.width) * frame_info.height);

        const auto encoded{jpegls_encoder::encode(source_to_encode, frame_info)};

        {
            // Copy the vector to ensure source buffer has a defined limit (resize() keeps memory)
            // that can be checked with address sanitizer.
            const vector source(encoded.cbegin(), encoded.cend() - 1);

            jpegls_decoder decoder{source, true};
            vector<byte> destination(decoder.get_destination_size());
            assert_expect_exception(jpegls_errc::need_more_data,
                                    [&decoder, &destination] { decoder.decode(destination); });
        }

        {
            const vector source(encoded.cbegin(), encoded.cend() - 2);
            jpegls_decoder decoder{source, true};
            vector<byte> destination(decoder.get_destination_size());

            assert_expect_exception(jpegls_errc::need_more_data,
                                    [&decoder, &destination] { decoder.decode(destination); });
        }

        {
            auto source(encoded);
            source[source.size() - 1] = byte{0x33};
            jpegls_decoder decoder{source, true};
            vector<byte> destination(decoder.get_destination_size());

            assert_expect_exception(jpegls_errc::end_of_image_marker_not_found,
                                    [&decoder, &destination] { decoder.decode(destination); });
        }
    }

    TEST_METHOD(decode_file_with_golomb_large_then_k_max_throws) // NOLINT
    {
        const auto source{read_file("fuzzy_input_golomb_16.jls")};

        jpegls_decoder decoder{source, true};

        const auto& frame_info{decoder.frame_info()};
        Assert::AreEqual(3, frame_info.component_count);
        Assert::AreEqual(16, frame_info.bits_per_sample);
        Assert::AreEqual(65516U, frame_info.height);
        Assert::AreEqual(1U, frame_info.width);

        vector<byte> destination(decoder.get_destination_size());

        assert_expect_exception(jpegls_errc::invalid_data,
                                [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(decode_file_with_no_start_byte_after_encoded_scan_throws) // NOLINT
    {
        const auto source{read_file("DataFiles/no_start_byte_after_encoded_scan.jls")};

        jpegls_decoder decoder{source, true};

        const auto& frame_info{decoder.frame_info()};
        Assert::AreEqual(3, frame_info.component_count);
        Assert::AreEqual(8, frame_info.bits_per_sample);
        Assert::AreEqual(1U, frame_info.height);
        Assert::AreEqual(1U, frame_info.width);

        vector<byte> destination(decoder.get_destination_size());

        assert_expect_exception(jpegls_errc::need_more_data,
                                [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(decode_file_with_missing_restart_marker_throws) // NOLINT
    {
        auto source{read_file("DataFiles/t8c0e0.jls")};

        // Insert a DRI marker segment to trigger that restart markers are used.
        jpeg_test_stream_writer stream_writer;
        stream_writer.write_define_restart_interval(10, 3);
        const auto it{source.begin() + 2};
        source.insert(it, stream_writer.buffer.cbegin(), stream_writer.buffer.cend());

        jpegls_decoder decoder{source, true};
        vector<byte> destination(decoder.get_destination_size());

        assert_expect_exception(jpegls_errc::restart_marker_not_found,
                                [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(decode_file_with_incorrect_restart_marker_throws) // NOLINT
    {
        auto source{read_file("DataFiles/test8_ilv_none_rm_7.jls")};

        // Change the first restart marker to the second.
        auto it{find_scan_header(source.begin(), source.end())};
        it = find_first_restart_marker(it + 1, source.end());
        ++it;
        *it = byte{0xD1};

        jpegls_decoder decoder{source, true};
        vector<byte> destination(decoder.get_destination_size());

        assert_expect_exception(jpegls_errc::restart_marker_not_found,
                                [&decoder, &destination] { decoder.decode(destination); });
    }

    TEST_METHOD(decode_file_with_extra_begin_bytes_for_restart_marker_code) // NOLINT
    {
        auto source{read_file("DataFiles/test8_ilv_none_rm_7.jls")};

        // Add additional 0xFF marker begin bytes
        auto it{find_scan_header(source.begin(), source.end())};
        it = find_first_restart_marker(it + 1, source.end());
        constexpr array extra_begin_bytes{byte{0xFF}, byte{0xFF}, byte{0xFF}, byte{0xFF},
                                          byte{0xFF}, byte{0xFF}, byte{0xFF}};
        source.insert(it, extra_begin_bytes.cbegin(), extra_begin_bytes.cend());

        const jpegls_decoder decoder{source, true};
        portable_anymap_file reference_file{
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.get_interleave_mode(), decoder.frame_info())};

        test_compliance(source, reference_file.image_data(), false);
    }

    TEST_METHOD(decode_file_that_ends_after_restart_marker_throws) // NOLINT
    {
        auto source{read_file("DataFiles/test8_ilv_none_rm_7.jls")};

        auto it{find_scan_header(source.begin(), source.end())};
        it = find_first_restart_marker(it + 1, source.end());

        // Copy the vector to ensure source buffer has a defined limit (resize() keeps memory)
        // that can be checked with address sanitizer.
        const vector too_small_source(source.begin(), it);

        jpegls_decoder decoder{too_small_source, true};
        vector<byte> destination(decoder.get_destination_size());

        assert_expect_exception(jpegls_errc::need_more_data,
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

    TEST_METHOD(at_comment_that_throws_returns_callback_failed_error) // NOLINT
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
        constexpr uint32_t width{99};
        constexpr uint32_t height{numeric_limits<uint16_t>::max() + 1U};
        writer.write_oversize_image_dimension(3, width, height);
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
        constexpr uint32_t width{99};
        constexpr uint32_t height{numeric_limits<uint16_t>::max()};
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
        constexpr uint32_t width{99};
        constexpr uint32_t height{numeric_limits<uint16_t>::max()};
        writer.write_start_of_frame_segment(width, height, 8, 3);
        writer.write_oversize_image_dimension(2, 10, 0);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());

        assert_expect_exception(jpegls_errc::invalid_parameter_width, [&decoder] { decoder.read_header(); });
    }

    TEST_METHOD(start_of_frame_changes_height_throws) // NOLINT
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        constexpr uint32_t width{};
        constexpr uint32_t height{numeric_limits<uint16_t>::max()};
        writer.write_oversize_image_dimension(2, width, 10);
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
        constexpr uint32_t width{numeric_limits<uint32_t>::max()};
        constexpr uint32_t height{numeric_limits<uint32_t>::max()};
        writer.write_oversize_image_dimension(4, width, height);
        constexpr size_t component_count{2};
        writer.write_start_of_frame_segment(0, 0, 8, component_count);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());
        decoder.read_header();

#if INTPTR_MAX == INT64_MAX
        Assert::AreEqual(component_count * numeric_limits<uint32_t>::max() * numeric_limits<uint32_t>::max(),
                         decoder.get_destination_size());
#elif INTPTR_MAX == INT32_MAX
        assert_expect_exception(jpegls_errc::parameter_value_not_supported,
                                [&decoder] { ignore = decoder.get_destination_size(); });
#else
#error Unknown pointer size or missing size macros!
#endif
    }

    TEST_METHOD(decode_to_buffer_with_uint16_size_works) // NOLINT
    {
        // These are compile time checks to detect issues with overloads that have similar conversions.
        constexpr frame_info frame_info{100, 100, 8, 1};
        const vector<byte> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        const vector encoded_source{
            jpegls_encoder::encode(source, frame_info, interleave_mode::none, encoding_options::even_destination_size)};

        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        vector<byte> destination(decoder.get_destination_size());

        void* data{destination.data()};
        const uint16_t size{static_cast<uint16_t>(destination.size())};

        // size is not a perfect match and needs a conversion.
        decoder.decode(data, size);
    }

    TEST_METHOD(abbreviated_format_mapping_table_count_after_read_header) // NOLINT
    {
        const vector<byte> table_data(255);
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_jpegls_preset_parameters_segment(1, 1, table_data, false);
        writer.write_marker(jpeg_marker_code::end_of_image);

        jpegls_decoder decoder;
        decoder.source(writer.buffer);
        Assert::AreEqual(compressed_data_format::unknown, decoder.compressed_data_format());

        decoder.read_header();
        const int32_t count{decoder.mapping_table_count()};
        Assert::AreEqual(1, count);
        Assert::AreEqual(compressed_data_format::abbreviated_table_specification, decoder.compressed_data_format());
    }

    TEST_METHOD(compressed_data_format_interchange) // NOLINT
    {
        constexpr frame_info frame_info{100, 100, 8, 1};
        const vector<byte> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        const vector encoded_source{
            jpegls_encoder::encode(source, frame_info, interleave_mode::none, encoding_options::even_destination_size)};

        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        vector<byte> destination(decoder.get_destination_size());

        void* data{destination.data()};
        const uint16_t size{static_cast<uint16_t>(destination.size())};

        decoder.decode(data, size);

        Assert::AreEqual(compressed_data_format::interchange, decoder.compressed_data_format());
    }

    TEST_METHOD(compressed_data_format_abbreviated_image_data) // NOLINT
    {
        constexpr frame_info frame_info{100, 100, 8, 1};
        const vector<byte> source(static_cast<size_t>(frame_info.width) * frame_info.height);

        jpegls_encoder encoder;
        encoder.frame_info(frame_info);
        encoder.set_mapping_table_id(0, 1);

        vector<byte> encoded_source(encoder.estimated_destination_size());
        encoder.destination(encoded_source);
        encoder.encode(source);

        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        vector<byte> destination(decoder.get_destination_size());

        void* data{destination.data()};
        const uint16_t size{static_cast<uint16_t>(destination.size())};

        decoder.decode(data, size);

        Assert::AreEqual(compressed_data_format::abbreviated_image_data, decoder.compressed_data_format());
    }

    TEST_METHOD(abbreviated_format_with_spiff_header_throws) // NOLINT
    {
        const vector<byte> table_data(255);
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();

        spiff_header header{};
        header.bits_per_sample = 8;
        header.color_space = spiff_color_space::rgb;
        header.component_count = 3;
        header.height = 1;
        header.width = 1;

        writer.write_spiff_header_segment(header);
        writer.write_spiff_end_of_directory_entry();
        writer.write_jpegls_preset_parameters_segment(1, 1, table_data, false);
        writer.write_marker(jpeg_marker_code::end_of_image);

        jpegls_decoder decoder;
        decoder.source(writer.buffer);
        decoder.read_spiff_header();

        assert_expect_exception(jpegls_errc::abbreviated_format_and_spiff_header_mismatch,
                                [&decoder] { ignore = decoder.read_header(); });
    }

    TEST_METHOD(mapping_table_count_after_decode_table_after_first_scan) // NOLINT
    {
        constexpr array data_h10{
            byte{0xFF}, byte{0xD8}, // Start of image (SOI) marker
            byte{0xFF}, byte{0xF7}, // Start of JPEG-LS frame (SOF 55) marker - marker segment follows
            byte{0x00}, byte{0x0E}, // Length of marker segment = 14 bytes including the length field
            byte{0x02},             // P = Precision = 2 bits per sample
            byte{0x00}, byte{0x04}, // Y = Number of lines = 4
            byte{0x00}, byte{0x03}, // X = Number of columns = 3
            byte{0x02},             // Nf = Number of components in the frame = 2
            byte{0x01},             // C1  = Component ID = 1 (first and only component)
            byte{0x11},             // Sub-sampling: H1 = 1, V1 = 1
            byte{0x00},             // Tq1 = 0 (this field is always 0)
            byte{0x02},             // C2  = Component ID = 2 (first and only component)
            byte{0x11},             // Sub-sampling: H1 = 1, V1 = 1
            byte{0x00},             // Tq1 = 0 (this field is always 0)

            byte{0xFF}, byte{0xF8},             // LSE - JPEG-LS preset parameters marker
            byte{0x00}, byte{0x11},             // Length of marker segment = 17 bytes including the length field
            byte{0x02},                         // ID = 2, mapping table
            byte{0x05},                         // TID = 5 Table identifier (arbitrary)
            byte{0x03},                         // Wt = 3 Width of table entry
            byte{0xFF}, byte{0xFF}, byte{0xFF}, // Entry for index 0
            byte{0xFF}, byte{0x00}, byte{0x00}, // Entry for index 1
            byte{0x00}, byte{0xFF}, byte{0x00}, // Entry for index 2
            byte{0x00}, byte{0x00}, byte{0xFF}, // Entry for index 3

            byte{0xFF}, byte{0xDA},             // Start of scan (SOS) marker
            byte{0x00}, byte{0x08},             // Length of marker segment = 8 bytes including the length field
            byte{0x01},                         // Ns = Number of components for this scan = 1
            byte{0x01},                         // C1 = Component ID = 1
            byte{0x05},                         // Tm 1  = Mapping table identifier = 5
            byte{0x00},                         // NEAR = 0 (near-lossless max error)
            byte{0x00},                         // ILV = 0 (interleave mode = non-interleaved)
            byte{0x00},                         // Al = 0, Ah = 0 (no point transform)
            byte{0xDB}, byte{0x95}, byte{0xF0}, // 3 bytes of compressed image data

            byte{0xFF}, byte{0xF8},             // LSE - JPEG-LS preset parameters marker
            byte{0x00}, byte{0x11},             // Length of marker segment = 17 bytes including the length field
            byte{0x02},                         // ID = 2, mapping table
            byte{0x06},                         // TID = 6 Table identifier (arbitrary)
            byte{0x03},                         // Wt = 3 Width of table entry
            byte{0xFF}, byte{0xFF}, byte{0xFF}, // Entry for index 0
            byte{0xFF}, byte{0x00}, byte{0x00}, // Entry for index 1
            byte{0x00}, byte{0xFF}, byte{0x00}, // Entry for index 2
            byte{0x00}, byte{0x00}, byte{0xFF}, // Entry for index 3

            byte{0xFF}, byte{0xDA},             // Start of scan (SOS) marker
            byte{0x00}, byte{0x08},             // Length of marker segment = 8 bytes including the length field
            byte{0x01},                         // Ns = Number of components for this scan = 1
            byte{0x02},                         // C1 = Component ID = 2
            byte{0x06},                         // Tm 1  = Mapping table identifier = 6
            byte{0x00},                         // NEAR = 0 (near-lossless max error)
            byte{0x00},                         // ILV = 0 (interleave mode = non-interleaved)
            byte{0x00},                         // Al = 0, Ah = 0 (no point transform)
            byte{0xDB}, byte{0x95}, byte{0xF0}, // 3 bytes of compressed image data

            byte{0xFF}, byte{0xD9} // End of image (EOI) marker
        };

        jpegls_decoder decoder;
        decoder.source(data_h10);
        decoder.read_header();

        vector<byte> destination(decoder.get_destination_size());
        decoder.decode(destination);

        const int32_t count{decoder.mapping_table_count()};
        Assert::AreEqual(2, count);

        Assert::AreEqual(5, decoder.get_mapping_table_id(0));
        Assert::AreEqual(6, decoder.get_mapping_table_id(1));
    }

    TEST_METHOD(invalid_table_id_throws) // NOLINT
    {
        const std::vector<std::byte> table_data(255);
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_jpegls_preset_parameters_segment(0, 1, table_data, false);

        jpegls_decoder decoder;
        decoder.source(writer.buffer);
        assert_expect_exception(jpegls_errc::invalid_parameter_mapping_table_id, [&decoder] { decoder.read_header(); });
    }

    TEST_METHOD(duplicate_table_id_throws) // NOLINT
    {
        const std::vector<std::byte> table_data(255);
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        writer.write_jpegls_preset_parameters_segment(1, 1, table_data, false);
        writer.write_start_of_frame_segment(1, 1, 8, 3);
        writer.write_jpegls_preset_parameters_segment(1, 1, table_data, false);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer);
        assert_expect_exception(jpegls_errc::invalid_parameter_mapping_table_id, [&decoder] { decoder.read_header(); });
    }

    TEST_METHOD(mapping_table_id_returns_zero) // NOLINT
    {
        const auto encoded_source{read_file("DataFiles/t8c0e0.jls")};

        jpegls_decoder decoder(encoded_source, true);
        vector<byte> decoded_destination(decoder.get_destination_size());

        decoder.decode(decoded_destination);

        Assert::AreEqual(0, decoder.get_mapping_table_id(0));
        Assert::AreEqual(0, decoder.get_mapping_table_id(1));
        Assert::AreEqual(0, decoder.get_mapping_table_id(2));
    }

    TEST_METHOD(mapping_table_id_for_invalid_component_throws) // NOLINT
    {
        const auto encoded_source{read_file("DataFiles/t8c0e0.jls")};

        jpegls_decoder decoder(encoded_source, true);
        vector<byte> decoded_destination(decoder.get_destination_size());

        decoder.decode(decoded_destination);

        assert_expect_exception(jpegls_errc::invalid_argument, [&decoder] { ignore = decoder.get_mapping_table_id(3); });
    }

    TEST_METHOD(mapping_table_id_before_decode_throws) // NOLINT
    {
        const auto encoded_source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder(encoded_source, true);

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.get_mapping_table_id(0); });
    }

    TEST_METHOD(mapping_table_index_before_decode_throws) // NOLINT
    {
        const auto encoded_source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder(encoded_source, true);

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.find_mapping_table_index(3); });
    }

    TEST_METHOD(mapping_table_index_invalid_index_throws) // NOLINT
    {
        const auto encoded_source{read_file("DataFiles/t8c0e0.jls")};

        jpegls_decoder decoder(encoded_source, true);
        vector<byte> decoded_destination(decoder.get_destination_size());
        decoder.decode(decoded_destination);

        assert_expect_exception(jpegls_errc::invalid_argument, [&decoder] { ignore = decoder.find_mapping_table_index(0); });
        assert_expect_exception(jpegls_errc::invalid_argument, [&decoder] { ignore = decoder.find_mapping_table_index(256); });
    }

    TEST_METHOD(mapping_table_count_before_decode_throws) // NOLINT
    {
        const auto encoded_source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder(encoded_source, true);

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.mapping_table_count(); });
    }

    TEST_METHOD(mapping_table_info_before_decode_throws) // NOLINT
    {
        const auto encoded_source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder(encoded_source, true);

        assert_expect_exception(jpegls_errc::invalid_operation, [&decoder] { ignore = decoder.get_mapping_table_info(0); });
    }

    TEST_METHOD(mapping_table_before_decode_throws) // NOLINT
    {
        const auto encoded_source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder(encoded_source, true);
        vector<byte> table(1000);

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&decoder, &table] { decoder.get_mapping_table_data(0, table); });
    }

    TEST_METHOD(mapping_table_invalid_index_throws) // NOLINT
    {
        const auto encoded_source{read_file("DataFiles/t8c0e0.jls")};

        jpegls_decoder decoder(encoded_source, true);
        vector<byte> decoded_destination(decoder.get_destination_size());
        decoder.decode(decoded_destination);
        vector<byte> table(1000);

        assert_expect_exception(jpegls_errc::invalid_argument,
                                [&decoder, &table] { decoder.get_mapping_table_data(0, table); });
    }

    TEST_METHOD(read_header_non_8_or_16_bit_with_color_transformation_throws) // NOLINT
    {
        const auto jpegls_data{read_file("land10-10bit-rgb-hp3-invalid.jls")};

        jpegls_decoder decoder{jpegls_data, false};

        assert_expect_exception(jpegls_errc::invalid_parameter_color_transformation, [&decoder] { decoder.read_header(); });
    }

private:
    // ReSharper disable CppPassValueParameterByConstReference
    [[nodiscard]]
    static vector<byte>::iterator find_scan_header(const vector<byte>::iterator begin,
                                                   const vector<byte>::iterator end) noexcept
    // ReSharper restore CppPassValueParameterByConstReference
    {
        constexpr byte start_of_scan{0xDA};

        for (auto it{begin}; it < end  - 1; ++it)
        {
            if (*it == byte{0xFF} && *(it + 1) == start_of_scan)
                return it;
        }

        return end;
    }

    // ReSharper disable CppPassValueParameterByConstReference
    [[nodiscard]]
    static vector<byte>::iterator find_first_restart_marker(const vector<byte>::iterator begin,
                                                            const vector<byte>::iterator end) noexcept
    // ReSharper restore CppPassValueParameterByConstReference
    {
        constexpr byte first_restart_marker{0xD0};

        for (auto it{begin}; it != end; ++it)
        {
            if (*it == byte{0xFF} && it + 1 != end && *(it + 1) == first_restart_marker)
                return it;
        }

        return end;
    }

    [[nodiscard]]
    static vector<byte> create_default_pc_parameters_segment()
    {
        vector<byte> segment;

        segment.push_back(byte{0xFF});
        segment.push_back(static_cast<byte>(jpeg_marker_code::jpegls_preset_parameters));
        push_back(segment, static_cast<uint16_t>(11 + sizeof(uint16_t)));
        segment.push_back(static_cast<byte>(jpegls_preset_parameters_type::preset_coding_parameters));
        push_back(segment, uint16_t{0});
        push_back(segment, uint16_t{0});
        push_back(segment, uint16_t{0});
        push_back(segment, uint16_t{0});
        push_back(segment, uint16_t{0});

        return segment;
    }

    static void insert_pc_parameters_segments(vector<byte> & jpegls_source, const int component_count)
    {
        const auto pcp_segment{create_default_pc_parameters_segment()};

        auto it{jpegls_source.begin()};
        for (int i{}; i != component_count; ++i)
        {
            it = find_scan_header(it, jpegls_source.end());
            it = jpegls_source.insert(it, pcp_segment.cbegin(), pcp_segment.cend());
            it += static_cast<vector<byte>::difference_type>(pcp_segment.size() + 2U);
        }
    }

    static void oversize_image_dimension_bad_segment_size_throws(const uint32_t number_of_bytes)
    {
        jpeg_test_stream_writer writer;
        writer.write_start_of_image();
        constexpr uint32_t height{numeric_limits<uint16_t>::max()};
        constexpr uint32_t width{0};
        writer.write_oversize_image_dimension(number_of_bytes, width, 10, true);
        writer.write_start_of_frame_segment(width, height, 8, 3);
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder;
        decoder.source(writer.buffer.data(), writer.buffer.size());

        assert_expect_exception(jpegls_errc::invalid_marker_segment_size, [&decoder] { decoder.read_header(); });
    }

    static void decode_image_with_too_small_buffer_throws(const char* image_filename, const uint32_t stride = 0, const uint32_t too_small_byte_count = 1)
    {
        const auto source{read_file(image_filename)};

        jpegls_decoder decoder{source, true};
        vector<byte> destination(decoder.get_destination_size(stride) - too_small_byte_count);

        assert_expect_exception(jpegls_errc::invalid_argument_size,
                                [&decoder, &destination, &stride] { decoder.decode(destination, stride); });
    }
};

} // namespace charls::test
