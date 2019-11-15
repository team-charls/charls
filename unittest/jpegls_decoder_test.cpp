// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include <vector>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
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

    TEST_METHOD(frame_info_without_read_header)
    {
        jpegls_decoder decoder;

        const vector<uint8_t> source(2000);
        decoder.source(source);

        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] { static_cast<void>(decoder.frame_info()); });
    }

    TEST_METHOD(interleave_mode_without_read_header)
    {
        jpegls_decoder decoder;

        const vector<uint8_t> source(2000);
        decoder.source(source);

        assert_expect_exception(jpegls_errc::invalid_operation,
            [&] { static_cast<void>(decoder.interleave_mode()); });
    }

    TEST_METHOD(near_lossless_without_read_header)
    {
        jpegls_decoder decoder;

        const vector<uint8_t> source(2000);
        decoder.source(source);

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

    TEST_METHOD(decode_reference_file_from_buffer)
    {
        vector<uint8_t> buffer{read_file("DataFiles/T8C0E0.JLS")};

        jpegls_decoder decoder;
        decoder.source(buffer.data(), buffer.size())
               .read_header();

        vector<uint8_t> destination(decoder.destination_size());
        decoder.decode(destination);

        portable_anymap_file reference_file = read_anymap_reference_file("DataFiles/TEST8.PPM", decoder.interleave_mode(), decoder.frame_info());

        const auto& reference_image_data = reference_file.image_data();
        for (size_t i = 0; i < destination.size(); ++i)
        {
            Assert::AreEqual(reference_image_data[i], destination[i]);
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
        jpegls_decoder decoder;

        const vector<uint8_t> buffer = create_test_spiff_header();
        decoder.source(buffer);

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

    TEST_METHOD(simple_decode)
    {
        const vector<uint8_t> encoded_source{read_file("DataFiles/T8C0E0.JLS")};

        vector<uint8_t> decoded_destination;
        const auto frame_info{jpegls_decoder::decode(encoded_source, decoded_destination)};

        Assert::AreEqual(3, frame_info.component_count);
        Assert::AreEqual(8, frame_info.bits_per_sample);
        Assert::AreEqual(256U, frame_info.height);
        Assert::AreEqual(256U, frame_info.width);
    }
};

} // namespace CharLSUnitTest
