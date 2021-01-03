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
using std::tie;
using std::vector;
using namespace charls_test;

namespace {

charls::jpegls_decoder create_decoder(const vector<uint8_t>& source)
{
    return {source, true};
}

} // namespace

namespace charls { namespace test {

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
        const array<uint8_t, 10> buffer{};
        decoder3.source(buffer.data(), buffer.size());
        decoder3 = std::move(decoder2);
    }

    TEST_METHOD(set_source_twice) // NOLINT
    {
        jpegls_decoder decoder;

        vector<uint8_t> source(2000);
        decoder.source(source);
        assert_expect_exception(jpegls_errc::invalid_operation, [&] { decoder.source(source); });
    }

    TEST_METHOD(read_spiff_header_without_source) // NOLINT
    {
        jpegls_decoder decoder;

        assert_expect_exception(jpegls_errc::invalid_operation, [&] { decoder.read_spiff_header(); });
    }

    TEST_METHOD(destination_size_without_reading_header) // NOLINT
    {
        jpegls_decoder decoder;

        assert_expect_exception(jpegls_errc::invalid_operation, [&] { static_cast<void>(decoder.destination_size()); });
    }

    TEST_METHOD(read_header_without_source) // NOLINT
    {
        jpegls_decoder decoder;

        assert_expect_exception(jpegls_errc::invalid_operation, [&] { decoder.read_header(); });
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

    TEST_METHOD(interleave_mode_without_read_header) // NOLINT
    {
        const vector<uint8_t> source(2000);
        jpegls_decoder decoder{source, false};

        assert_expect_exception(jpegls_errc::invalid_operation, [&] { static_cast<void>(decoder.interleave_mode()); });
    }

    TEST_METHOD(near_lossless_without_read_header) // NOLINT
    {
        const vector<uint8_t> source(2000);
        jpegls_decoder decoder{source, false};

        assert_expect_exception(jpegls_errc::invalid_operation, [&] { static_cast<void>(decoder.near_lossless()); });
    }

    TEST_METHOD(preset_coding_parameters_without_read_header) // NOLINT
    {
        jpegls_decoder decoder;

        const vector<uint8_t> source(2000);
        decoder.source(source);

        assert_expect_exception(jpegls_errc::invalid_operation,
                                [&] { static_cast<void>(decoder.preset_coding_parameters()); });
    }

    TEST_METHOD(destination_size) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder{source, true};

        constexpr size_t expected_destination_size{static_cast<size_t>(256) * 256 * 3};
        Assert::AreEqual(expected_destination_size, decoder.destination_size());
    }

    TEST_METHOD(destination_size_stride_interleave_none) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder{source, true};

        constexpr size_t stride{512};
        constexpr size_t expected_destination_size{stride * 256 * 3};
        Assert::AreEqual(expected_destination_size, decoder.destination_size(stride));
    }

    TEST_METHOD(destination_size_stride_interleave_line) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c1e0.jls")};

        const jpegls_decoder decoder{source, true};

        constexpr size_t stride{1024};
        constexpr size_t expected_destination_size{stride * 256};
        Assert::AreEqual(expected_destination_size, decoder.destination_size(stride));
    }

    TEST_METHOD(destination_size_stride_interleave_sample) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c2e0.jls")};

        const jpegls_decoder decoder{source, true};

        constexpr size_t stride = 1024;
        constexpr size_t expected_destination_size{stride * 256};
        Assert::AreEqual(expected_destination_size, decoder.destination_size(stride));
    }

    TEST_METHOD(decode_reference_file_from_buffer) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder{source, true};

        vector<uint8_t> destination(decoder.destination_size());
        decoder.decode(destination);

        portable_anymap_file reference_file =
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.interleave_mode(), decoder.frame_info());

        const auto& reference_image_data = reference_file.image_data();
        for (size_t i = 0; i < destination.size(); ++i)
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

        portable_anymap_file reference_file =
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.interleave_mode(), decoder.frame_info());

        const auto& reference_image_data = reference_file.image_data();
        for (size_t i = 0; i < destination.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination[i]);
        }
    }

    TEST_METHOD(decode_with_destination_as_return) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder{source, true};

        const auto destination = decoder.decode<vector<uint8_t>>();

        portable_anymap_file reference_file =
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.interleave_mode(), decoder.frame_info());

        const auto& reference_image_data = reference_file.image_data();
        for (size_t i{}; i < destination.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination[i]);
        }
    }

    TEST_METHOD(decode_with_16_bit_destination_as_return) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        const jpegls_decoder decoder{source, true};

        const auto destination = decoder.decode<vector<uint16_t>>();

        portable_anymap_file reference_file =
            read_anymap_reference_file("DataFiles/test8.ppm", decoder.interleave_mode(), decoder.frame_info());

        const auto& reference_image_data{reference_file.image_data()};
        const auto* destination_as_bytes{reinterpret_cast<const uint8_t*>(destination.data())};
        for (size_t i{}; i < reference_image_data.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination_as_bytes[i]);
        }
    }

    TEST_METHOD(decode_without_reading_header) // NOLINT
    {
        jpegls_decoder decoder;

        vector<uint8_t> buffer(1000);
        assert_expect_exception(jpegls_errc::invalid_operation, [&] { decoder.decode(buffer); });
    }

    TEST_METHOD(decode_reference_to_mapping_table_selector) // NOLINT
    {
        jpeg_test_stream_writer writer;

        writer.write_start_of_image();
        writer.write_start_of_frame_segment(10, 10, 8, 3);
        writer.mapping_table_selector = 1;
        writer.write_start_of_scan_segment(0, 1, 0, interleave_mode::none);

        jpegls_decoder decoder{writer.buffer, false};

        assert_expect_exception(jpegls_errc::parameter_value_not_supported, [&] { decoder.read_header(); });
    }

    TEST_METHOD(read_spiff_header) // NOLINT
    {
        const vector<uint8_t> source = create_test_spiff_header();
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
        static_cast<void>(decoder.read_spiff_header(ec));

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

    TEST_METHOD(read_header_twice) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/t8c0e0.jls")};

        jpegls_decoder decoder{source, true};

        assert_expect_exception(jpegls_errc::invalid_operation, [&] { static_cast<void>(decoder.read_header()); });
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

    TEST_METHOD(decode_file_with_ff_in_entropy_data) // NOLINT
    {
        const vector<uint8_t> source{read_file("ff_in_entropy_data.jls")};

        jpegls_decoder decoder{source, true};

        const auto& frame_info{decoder.frame_info()};
        Assert::AreEqual(1, frame_info.component_count);
        Assert::AreEqual(12, frame_info.bits_per_sample);
        Assert::AreEqual(1216U, frame_info.height);
        Assert::AreEqual(968U, frame_info.width);

        vector<uint8_t> destination(decoder.destination_size());

        assert_expect_exception(jpegls_errc::invalid_encoded_data, [&] { static_cast<void>(decoder.decode(destination)); });
    }

    TEST_METHOD(decode_file_with_golomb_large_then_k_max) // NOLINT
    {
        const vector<uint8_t> source{read_file("fuzzy_input_golomb_16.jls")};

        jpegls_decoder decoder{source, true};

        const auto& frame_info{decoder.frame_info()};
        Assert::AreEqual(3, frame_info.component_count);
        Assert::AreEqual(16, frame_info.bits_per_sample);
        Assert::AreEqual(65516U, frame_info.height);
        Assert::AreEqual(1U, frame_info.width);

        vector<uint8_t> destination(decoder.destination_size());

        assert_expect_exception(jpegls_errc::invalid_encoded_data, [&] { static_cast<void>(decoder.decode(destination)); });
    }


private:
    static vector<uint8_t>::iterator find_scan_header(const vector<uint8_t>::iterator& begin,
                                                      const vector<uint8_t>::iterator& end) noexcept
    {
        constexpr uint8_t start_of_scan{0xDA};

        for (auto it{begin}; it != end; ++it)
        {
            if (*it == 0xFF && it + 1 != end && *(it + 1) == start_of_scan)
                return it;
        }

        return end;
    }

    static vector<uint8_t> create_default_pc_parameters_segment()
    {
        vector<uint8_t> segment;

        segment.push_back(static_cast<uint8_t>(0xFF));
        segment.push_back(static_cast<uint8_t>(jpeg_marker_code::jpegls_preset_parameters));
        push_back(segment, static_cast<uint16_t>(11 + sizeof(uint16_t)));
        segment.push_back(static_cast<uint8_t>(jpegls_preset_parameters_type::preset_coding_parameters));
        push_back(segment, static_cast<uint16_t>(0));
        push_back(segment, static_cast<uint16_t>(0));
        push_back(segment, static_cast<uint16_t>(0));
        push_back(segment, static_cast<uint16_t>(0));
        push_back(segment, static_cast<uint16_t>(0));

        return segment;
    }

    static void insert_pc_parameters_segments(vector<uint8_t> & jpegls_source, const int component_count)
    {
        const auto pcp_segment{create_default_pc_parameters_segment()};

        auto it{jpegls_source.begin()};
        for (int i{}; i < component_count; ++i)
        {
            it = find_scan_header(it, jpegls_source.end());
            it = jpegls_source.insert(it, pcp_segment.cbegin(), pcp_segment.cend());
            it += static_cast<vector<uint8_t>::difference_type>(pcp_segment.size() + 2U);
        }
    }
};

}} // namespace charls::test
