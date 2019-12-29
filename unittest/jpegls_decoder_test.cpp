// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include <tuple>
#include <vector>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::error_code;
using std::tie;
using std::vector;
using namespace charls;
using namespace charls_test;

namespace CharLSUnitTest {

// clang-format off

TEST_CLASS(jpegls_decoder_test)
{
public:
    TEST_METHOD(create_destroy)
    {
        // ReSharper disable once CppLocalVariableWithNonTrivialDtorIsNeverUsed
        jpegls_decoder decoder;
    }

    TEST_METHOD(create_and_move)
    {
        jpegls_decoder decoder1;

        jpegls_decoder decoder2(std::move(decoder1));

        // ReSharper disable once CppLocalVariableWithNonTrivialDtorIsNeverUsed
        jpegls_decoder decoder3 = std::move(decoder2);
    }

    TEST_METHOD(set_source_twice)
    {
        jpegls_decoder decoder;

        vector<uint8_t> source(2000);
        decoder.source(source);
        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] { decoder.source(source); });
    }

    TEST_METHOD(read_spiff_header_without_source)
    {
        jpegls_decoder decoder;

        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] {
                    bool header_found;
                    decoder.read_spiff_header(header_found);
                });
    }

    TEST_METHOD(destination_size_without_reading_header)
    {
        jpegls_decoder decoder;

        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] { static_cast<void>(decoder.destination_size()); });
    }

    TEST_METHOD(read_header_without_source)
    {
        jpegls_decoder decoder;

        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] { decoder.read_header(); });
    }

    TEST_METHOD(read_header_from_non_jpegls_data)
    {
        const vector<uint8_t> source(100);
        jpegls_decoder decoder{source};

        error_code ec;
        decoder.read_header(ec);

        Assert::IsTrue(ec == jpegls_errc::jpeg_marker_start_byte_not_found);
    }

    TEST_METHOD(frame_info_without_read_header)
    {
        const vector<uint8_t> source(2000);
        jpegls_decoder decoder{source};

        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] { static_cast<void>(decoder.frame_info()); });
    }

    TEST_METHOD(interleave_mode_without_read_header)
    {
        const vector<uint8_t> source(2000);
        jpegls_decoder decoder{source};

        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] { static_cast<void>(decoder.interleave_mode()); });
    }

    TEST_METHOD(near_lossless_without_read_header)
    {
        const vector<uint8_t> source(2000);
        jpegls_decoder decoder{source};

        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] { static_cast<void>(decoder.near_lossless()); });
    }

    TEST_METHOD(preset_coding_parameters_without_read_header)
    {
        jpegls_decoder decoder;

        const vector<uint8_t> source(2000);
        decoder.source(source);

        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] { static_cast<void>(decoder.preset_coding_parameters()); });
    }

    TEST_METHOD(destination_size)
    {
        const vector<uint8_t> source{read_file("DataFiles/T8C0E0.JLS")};

        jpegls_decoder decoder{source};
        decoder.read_header();

        constexpr size_t expected_destination_size{256 * 256 * 3};
        Assert::AreEqual(expected_destination_size, decoder.destination_size());
    }

    TEST_METHOD(destination_size_stride_interleave_none)
    {
        const vector<uint8_t> source{read_file("DataFiles/T8C0E0.JLS")};

        jpegls_decoder decoder{source};
        decoder.read_header();

        constexpr uint32_t stride = 512;
        constexpr size_t expected_destination_size{stride * 256 * 3};
        Assert::AreEqual(expected_destination_size, decoder.destination_size(stride));
    }

    TEST_METHOD(destination_size_stride_interleave_line)
    {
        const vector<uint8_t> source{read_file("DataFiles/T8C1E0.JLS")};

        jpegls_decoder decoder{source};
        decoder.read_header();

        constexpr uint32_t stride = 1024;
        constexpr size_t expected_destination_size{stride * 256};
        Assert::AreEqual(expected_destination_size, decoder.destination_size(stride));
    }

    TEST_METHOD(destination_size_stride_interleave_sample)
    {
        const vector<uint8_t> source{read_file("DataFiles/T8C2E0.JLS")};

        jpegls_decoder decoder{source};
        decoder.read_header();

        constexpr uint32_t stride = 1024;
        constexpr size_t expected_destination_size{stride * 256};
        Assert::AreEqual(expected_destination_size, decoder.destination_size(stride));
    }

    TEST_METHOD(decode_reference_file_from_buffer)
    {
        const vector<uint8_t> source{read_file("DataFiles/T8C0E0.JLS")};

        jpegls_decoder decoder{source};
        decoder.read_header();

        vector<uint8_t> destination(decoder.destination_size());
        decoder.decode(destination);

        portable_anymap_file reference_file = read_anymap_reference_file("DataFiles/TEST8.PPM", decoder.interleave_mode(), decoder.frame_info());

        const auto& reference_image_data = reference_file.image_data();
        for (size_t i = 0; i < destination.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination[i]);
        }
    }

    TEST_METHOD(decode_with_destination_as_return)
    {
        const vector<uint8_t> source{read_file("DataFiles/T8C0E0.JLS")};

        jpegls_decoder decoder{source};
        decoder.read_header();

        const auto destination = decoder.decode<vector<uint8_t>>();

        portable_anymap_file reference_file = read_anymap_reference_file("DataFiles/TEST8.PPM", decoder.interleave_mode(), decoder.frame_info());

        const auto& reference_image_data = reference_file.image_data();
        for (size_t i = 0; i < destination.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination[i]);
        }
    }

    TEST_METHOD(decode_with_16bit_destination_as_return)
    {
        const vector<uint8_t> source{read_file("DataFiles/T8C0E0.JLS")};

        jpegls_decoder decoder{source};
        decoder.read_header();

        const auto destination = decoder.decode<vector<uint16_t>>();

        portable_anymap_file reference_file = read_anymap_reference_file("DataFiles/TEST8.PPM", decoder.interleave_mode(), decoder.frame_info());

        const auto& reference_image_data = reference_file.image_data();
        const auto* destination_as_bytes = reinterpret_cast<const uint8_t*>(destination.data());
        for (size_t i = 0; i < reference_image_data.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination_as_bytes[i]);
        }
    }

    TEST_METHOD(decode_without_reading_header)
    {
        jpegls_decoder decoder;

        vector<uint8_t> buffer(1000);
        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] { decoder.decode(buffer); });
    }

    TEST_METHOD(read_spiff_header)
    {
        const vector<uint8_t> source = create_test_spiff_header();
        const jpegls_decoder decoder{source};

        bool found;
        const auto header = decoder.read_spiff_header(found);

        Assert::IsTrue(found);
        Assert::AreEqual(static_cast<int32_t>(spiff_profile_id::none), static_cast<int32_t>(header.profile_id));
        Assert::AreEqual(3, header.component_count);
        Assert::AreEqual(800U, header.height);
        Assert::AreEqual(600U, header.width);
        Assert::AreEqual(static_cast<int32_t>(spiff_color_space::rgb), static_cast<int32_t>(header.color_space));
        Assert::AreEqual(8, header.bits_per_sample);
        Assert::AreEqual(static_cast<int32_t>(spiff_compression_type::jpeg_ls), static_cast<int32_t>(header.compression_type));
        Assert::AreEqual(static_cast<int32_t>(spiff_resolution_units::dots_per_inch), static_cast<int32_t>(header.resolution_units));
        Assert::AreEqual(96U, header.vertical_resolution);
        Assert::AreEqual(1024U, header.horizontal_resolution);
    }

    TEST_METHOD(read_spiff_header_from_non_jpegls_data)
    {
        const vector<uint8_t> source(100);
        const jpegls_decoder decoder{source};

        bool found;
        error_code ec;
        static_cast<void>(decoder.read_spiff_header(found, ec));

        Assert::IsTrue(ec == jpegls_errc::jpeg_marker_start_byte_not_found);
    }

    TEST_METHOD(read_spiff_header_from_jpegls_without_spiff)
    {
        const vector<uint8_t> source{read_file("DataFiles/T8C0E0.JLS")};

        jpegls_decoder decoder{source};

        bool found;
        static_cast<void>(decoder.read_spiff_header(found));
        Assert::IsFalse(found);

        decoder.read_header();
        const frame_info frame_info{decoder.frame_info()};

        Assert::AreEqual(3, frame_info.component_count);
        Assert::AreEqual(8, frame_info.bits_per_sample);
        Assert::AreEqual(256U, frame_info.height);
        Assert::AreEqual(256U, frame_info.width);
    }

    TEST_METHOD(read_header_twice)
    {
        const vector<uint8_t> source{read_file("DataFiles/T8C0E0.JLS")};

        jpegls_decoder decoder{source};

        decoder.read_header();

        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] { static_cast<void>(decoder.read_header()); });
    }

    TEST_METHOD(simple_decode)
    {
        const vector<uint8_t> encoded_source{read_file("DataFiles/T8C0E0.JLS")};

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

    TEST_METHOD(simple_decode_to_uint16_buffer)
    {
        const vector<uint8_t> encoded_source{read_file("DataFiles/T8C0E0.JLS")};

        vector<uint16_t> decoded_destination;
        frame_info frame_info;
        interleave_mode interleave_mode;
        tie(frame_info, interleave_mode) = jpegls_decoder::decode(encoded_source, decoded_destination);

        Assert::AreEqual(3, frame_info.component_count);
        Assert::AreEqual(8, frame_info.bits_per_sample);
        Assert::AreEqual(256U, frame_info.height);
        Assert::AreEqual(256U, frame_info.width);
        Assert::AreEqual(interleave_mode::none, interleave_mode);

        const size_t expected_size = static_cast<size_t>(frame_info.height) * frame_info.width * frame_info.component_count;
        Assert::AreEqual(expected_size, decoded_destination.size() * sizeof(uint16_t));
    }

    TEST_METHOD(decode_file_with_ff_in_entropy_data)
    {
        const vector<uint8_t> source{read_file("ff_in_entropy_data.jls")};

        jpegls_decoder decoder{source};
        decoder.read_header();

        const auto frame_info{decoder.frame_info()};
        Assert::AreEqual(1, frame_info.component_count);
        Assert::AreEqual(12, frame_info.bits_per_sample);
        Assert::AreEqual(1216U, frame_info.height);
        Assert::AreEqual(968U, frame_info.width);

        vector<uint8_t> destination(decoder.destination_size());

        assert_expect_exception(jpegls_errc::invalid_encoded_data,
            [&] { static_cast<void>(decoder.decode(destination)); });
    }
};

} // namespace CharLSUnitTest
