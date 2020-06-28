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
using std::system_error;
using std::vector;

namespace charls {
namespace test {

// clang-format off

TEST_CLASS(JpegStreamReaderTest)
{
public:
    TEST_METHOD(ReadHeaderFromToSmallInputBuffer) // NOLINT
    {
        array<uint8_t, 1> buffer{};

        const byte_stream_info byteStream = FromByteArray(buffer.data(), 0);
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::source_buffer_too_small), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderFromBufferPrecededWithFillBytes) // NOLINT
    {
        JpegTestStreamWriter writer;

        writer.buffer.push_back(0xFF);
        writer.write_start_of_image();

        writer.buffer.push_back(0xFF);
        writer.write_start_of_frame_segment(1, 1, 2, 1);

        writer.buffer.push_back(0xFF);
        writer.WriteStartOfScanSegment(0, 1, 128, charls::interleave_mode::none);

        const byte_stream_info byteStream = FromByteArray(writer.buffer.data(), writer.buffer.size());
        jpeg_stream_reader reader(byteStream);

        reader.read_header(); // if it doesn't throw test is passed.
    }

    TEST_METHOD(ReadHeaderFromBufferNotStartingWithFFShouldThrow) // NOLINT
    {
        vector<uint8_t> buffer;
        buffer.push_back(0x0F);
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xFF);
        buffer.push_back(0xDA); // SOS: Marks the start of scan.

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::jpeg_marker_start_byte_not_found), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithApplicationData) // NOLINT
    {
        ReadHeaderWithApplicationData(0);
        ReadHeaderWithApplicationData(1);
        ReadHeaderWithApplicationData(2);
        ReadHeaderWithApplicationData(3);
        ReadHeaderWithApplicationData(4);
        ReadHeaderWithApplicationData(5);
        ReadHeaderWithApplicationData(6);
        ReadHeaderWithApplicationData(7);
        ReadHeaderWithApplicationData(8);
        ReadHeaderWithApplicationData(9);
        ReadHeaderWithApplicationData(10);
        ReadHeaderWithApplicationData(11);
        ReadHeaderWithApplicationData(12);
        ReadHeaderWithApplicationData(13);
        ReadHeaderWithApplicationData(14);
        ReadHeaderWithApplicationData(15);
    }

    TEST_METHOD(ReadHeaderWithJpegLSExtendedFrameShouldThrow) // NOLINT
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF9); // SOF_57: Marks the start of a JPEG-LS extended (ISO/IEC 14495-2) encoded frame.

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::encoding_not_supported), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderJpegLSPresetParameterSegment) // NOLINT
    {
        vector<uint8_t> source(100);
        const byte_stream_info sourceInfo = FromByteArray(source.data(), source.size());

        charls::jpeg_stream_writer writer(sourceInfo);
        writer.write_start_of_image();

        const jpegls_pc_parameters presets{1, 2, 3, 4, 5};
        writer.write_jpegls_preset_parameters_segment(presets);
        writer.write_start_of_frame_segment(1, 1, 2, 1);
        writer.write_start_of_scan_segment(1, 0, interleave_mode::none);

        const byte_stream_info destinationInfo = FromByteArray(source.data(), source.size());
        jpeg_stream_reader reader(destinationInfo);

        reader.read_header();
        const auto& actual = reader.preset_coding_parameters();

        Assert::AreEqual(presets.maximum_sample_value, actual.maximum_sample_value);
        Assert::AreEqual(presets.reset_value, actual.reset_value);
        Assert::AreEqual(presets.threshold1, actual.threshold1);
        Assert::AreEqual(presets.threshold2, actual.threshold2);
        Assert::AreEqual(presets.threshold3, actual.threshold3);
    }

    TEST_METHOD(ReadHeaderWithTooSmallJpegLSPresetParameterSegmentShouldThrow) // NOLINT
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF8); // LSE: Marks the start of a JPEG-LS preset parameters segment.
        buffer.push_back(0x00);
        buffer.push_back(0x02);
        buffer.push_back(0x01);

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooSmallJpegLSPresetParameterSegmentWithCodingParametersShouldThrow) // NOLINT
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF8); // LSE: Marks the start of a JPEG-LS preset parameters segment.
        buffer.push_back(0x00);
        buffer.push_back(0x0A);
        buffer.push_back(0x01);

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooLargeJpegLSPresetParameterSegmentWithCodingParametersShouldThrow) // NOLINT
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF8); // LSE: Marks the start of a JPEG-LS preset parameters segment.
        buffer.push_back(0x00);
        buffer.push_back(0x0C);
        buffer.push_back(0x01);

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(read_header_bad_jpegls_preset_coding_parameters_should_throw) // NOLINT
    {
        const jpegls_pc_parameters preset_coding_parameters{256, 0, 0, 0, 0};

        JpegTestStreamWriter writer;
        writer.write_start_of_image();
        writer.write_jpegls_preset_parameters_segment(preset_coding_parameters);
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.WriteStartOfScanSegment(0, 1, 127, charls::interleave_mode::none);
        const byte_stream_info source = FromByteArray(writer.buffer.data(), writer.buffer.size());

        jpeg_stream_reader reader(source);

        assert_expect_exception(jpegls_errc::invalid_parameter_jpegls_pc_parameters,
            [&](){reader.read_header();});
    }

    static void ReadHeaderWithJpegLSPresetParameterWithExtendedIdShouldThrow(const uint8_t id)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF8); // LSE: Marks the start of a JPEG-LS preset parameters segment.
        buffer.push_back(0x00);
        buffer.push_back(0x03);
        buffer.push_back(id);

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::jpegls_preset_extended_parameter_type_not_supported), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithJpegLSPresetParameterWithExtendedIdShouldThrow) // NOLINT
    {
        ReadHeaderWithJpegLSPresetParameterWithExtendedIdShouldThrow(0x5);
        ReadHeaderWithJpegLSPresetParameterWithExtendedIdShouldThrow(0x6);
        ReadHeaderWithJpegLSPresetParameterWithExtendedIdShouldThrow(0x7);
        ReadHeaderWithJpegLSPresetParameterWithExtendedIdShouldThrow(0x8);
        ReadHeaderWithJpegLSPresetParameterWithExtendedIdShouldThrow(0x9);
        ReadHeaderWithJpegLSPresetParameterWithExtendedIdShouldThrow(0xA);
        ReadHeaderWithJpegLSPresetParameterWithExtendedIdShouldThrow(0xC);
        ReadHeaderWithJpegLSPresetParameterWithExtendedIdShouldThrow(0xD);
    }

    TEST_METHOD(ReadHeaderWithTooSmallSegmentSizeShouldThrow) // NOLINT
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF7); // SOF_55: Marks the start of JPEG-LS extended scan.
        buffer.push_back(0x00);
        buffer.push_back(0x01);
        buffer.push_back(0xFF);
        buffer.push_back(0xDA); // SOS: Marks the start of scan.

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooSmallStartOfFrameShouldThrow) // NOLINT
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF7); // SOF_55: Marks the start of JPEG-LS extended scan.
        buffer.push_back(0x00);
        buffer.push_back(0x07);

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooSmallStartOfFrameInComponentInfoShouldThrow) // NOLINT
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF7); // SOF_55: Marks the start of JPEG-LS extended scan.
        buffer.push_back(0x00);
        buffer.push_back(0x07);

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooLargeStartOfFrameShouldThrow) // NOLINT
    {
        JpegTestStreamWriter writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.buffer.push_back(0);
        writer.buffer[5]++;
        const byte_stream_info byteStream = FromByteArray(writer.buffer.data(), writer.buffer.size());

        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(read_header_sos_before_sof_should_throw) // NOLINT
    {
        JpegTestStreamWriter writer;
        writer.write_start_of_image();
        writer.WriteStartOfScanSegment(0, 1, 128, charls::interleave_mode::none);
        const byte_stream_info source = FromByteArray(writer.buffer.data(), writer.buffer.size());

        jpeg_stream_reader reader(source);

        assert_expect_exception(jpegls_errc::unexpected_marker_found,
            [&](){reader.read_header();});
    }

    TEST_METHOD(read_header_extra_sof_should_throw) // NOLINT
    {
        JpegTestStreamWriter writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        const byte_stream_info source = FromByteArray(writer.buffer.data(), writer.buffer.size());

        jpeg_stream_reader reader(source);

        assert_expect_exception(jpegls_errc::duplicate_start_of_frame_marker,
            [&](){reader.read_header();});
    }

    TEST_METHOD(read_header_too_large_near_lossless_in_sos_should_throw) // NOLINT
    {
        JpegTestStreamWriter writer;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        writer.WriteStartOfScanSegment(0, 1, 128, charls::interleave_mode::none);
        const byte_stream_info source = FromByteArray(writer.buffer.data(), writer.buffer.size());

        jpeg_stream_reader reader(source);
        reader.read_header();

        assert_expect_exception(jpegls_errc::invalid_parameter_near_lossless,
            [&](){reader.read_start_of_scan();});
    }

    TEST_METHOD(read_header_too_large_near_lossless_in_sos_should_throw2) // NOLINT
    {
        const jpegls_pc_parameters preset_coding_parameters{200, 0, 0, 0, 0};

        JpegTestStreamWriter writer;
        writer.write_start_of_image();
        writer.write_jpegls_preset_parameters_segment(preset_coding_parameters);
        writer.write_start_of_frame_segment(512, 512, 8, 3);

        constexpr int bad_near_lossless = (200 / 2) + 1;
        writer.WriteStartOfScanSegment(0, 1, bad_near_lossless, charls::interleave_mode::none);
        const byte_stream_info source = FromByteArray(writer.buffer.data(), writer.buffer.size());

        jpeg_stream_reader reader(source);
        reader.read_header();

        assert_expect_exception(jpegls_errc::invalid_parameter_near_lossless,
            [&](){reader.read_start_of_scan();});
    }

    TEST_METHOD(ReadHeaderWithDuplicateComponentIdInStartOfFrameSegmentShouldThrow) // NOLINT
    {
        JpegTestStreamWriter writer;
        writer.componentIdOverride = 7;
        writer.write_start_of_image();
        writer.write_start_of_frame_segment(512, 512, 8, 3);
        const byte_stream_info byteStream = FromByteArray(writer.buffer.data(), writer.buffer.size());

        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::duplicate_component_id_in_sof_segment), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooSmallStartOfScanShouldThrow) // NOLINT
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF7); // SOF_55: Marks the start of JPEG-LS extended scan.
        buffer.push_back(0x00);
        buffer.push_back(0x08); // size
        buffer.push_back(0x08); // bits per sample
        buffer.push_back(0x00);
        buffer.push_back(0x01); // width
        buffer.push_back(0x00);
        buffer.push_back(0x01); // height
        buffer.push_back(0x01); // component count
        buffer.push_back(0xFF);
        buffer.push_back(0xDA); // SOS
        buffer.push_back(0x00);
        buffer.push_back(0x03);

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooSmallStartOfScanComponentCountShouldThrow) // NOLINT
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF7); // SOF_55: Marks the start of JPEG-LS extended scan.
        buffer.push_back(0x00);
        buffer.push_back(0x08); // size
        buffer.push_back(0x08); // bits per sample
        buffer.push_back(0x00);
        buffer.push_back(0x01); // width
        buffer.push_back(0x00);
        buffer.push_back(0x01); // height
        buffer.push_back(0x01); // component count
        buffer.push_back(0xFF);
        buffer.push_back(0xDA); // SOS
        buffer.push_back(0x00);
        buffer.push_back(0x07);
        buffer.push_back(0x01);

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithDirectlyEndOfImageShouldThrow) // NOLINT
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xD9); // EOI.

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::unexpected_end_of_image_marker), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithDuplicateStartOfImageShouldThrow) // NOLINT
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xD8); // SOI.

        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        try
        {
            reader.read_header();
            Assert::Fail();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::duplicate_start_of_image_marker), error.code().value());
        }
    }

    TEST_METHOD(ReadSpiffHeader) // NOLINT
    {
        ReadSpiffHeader(0);
    }

    TEST_METHOD(read_spiff_header_low_version_newer) // NOLINT
    {
        ReadSpiffHeader(1);
    }

    TEST_METHOD(read_spiff_header_high_version_to_new) // NOLINT
    {
        vector<uint8_t> buffer = create_test_spiff_header(3);
        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        spiff_header spiff_header{};
        bool spiff_header_found{};

        reader.read_header(&spiff_header, &spiff_header_found);

        Assert::IsFalse(spiff_header_found);
    }

    TEST_METHOD(read_spiff_header_without_end_of_directory) // NOLINT
    {
        vector<uint8_t> buffer = create_test_spiff_header(2, 0, false);
        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

        spiff_header spiff_header{};
        bool spiff_header_found{};

        reader.read_header(&spiff_header, &spiff_header_found);
        Assert::IsTrue(spiff_header_found);

        try
        {
            reader.read_header();
            Assert::Fail();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::missing_end_of_spiff_directory), error.code().value());
        }
    }

private:
    static void ReadSpiffHeader(const uint8_t low_version)
    {
        vector<uint8_t> buffer = create_test_spiff_header(2, low_version);
        const byte_stream_info byteStream = FromByteArray(buffer.data(), buffer.size());
        jpeg_stream_reader reader(byteStream);

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
        Assert::AreEqual(static_cast<int32_t>(spiff_compression_type::jpeg_ls), static_cast<int32_t>(spiff_header.compression_type));
        Assert::AreEqual(static_cast<int32_t>(spiff_resolution_units::dots_per_inch), static_cast<int32_t>(spiff_header.resolution_units));
        Assert::AreEqual(96U, spiff_header.vertical_resolution);
        Assert::AreEqual(1024U, spiff_header.horizontal_resolution);
    }

    static void ReadHeaderWithApplicationData(const uint8_t dataNumber)
    {
        JpegTestStreamWriter writer;
        writer.write_start_of_image();

        writer.buffer.push_back(0xFF);
        writer.buffer.push_back(0xE0 + dataNumber);
        writer.buffer.push_back(0x00);
        writer.buffer.push_back(0x02);

        writer.write_start_of_frame_segment(1, 1, 2, 1);
        writer.WriteStartOfScanSegment(0, 1, 128, charls::interleave_mode::none);

        const byte_stream_info byteStream = FromByteArray(writer.buffer.data(), writer.buffer.size());
        jpeg_stream_reader reader(byteStream);

        reader.read_header(); // if it doesn't throw test is passed.
    }
};

} // namespace test
} // namespace charls
