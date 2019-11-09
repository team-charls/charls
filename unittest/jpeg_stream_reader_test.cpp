// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include "../src/jpeg_stream_reader.h"
#include "../src/jpeg_stream_writer.h"
#include "jpeg_test_stream_writer.h"

#include <array>
#include <cstdint>
#include <vector>

using namespace charls;
using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;
using std::system_error;
using std::vector;

namespace CharLSUnitTest {

// clang-format off

TEST_CLASS(JpegStreamReaderTest)
{
public:
    TEST_METHOD(ReadHeaderFromToSmallInputBuffer)
    {
        array<uint8_t, 1> buffer{};

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), 0);
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::source_buffer_too_small), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderFromBufferPrecededWithFillBytes)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xFF);
        buffer.push_back(0xDA); // SOS: Marks the start of scan.

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        reader.ReadHeader(); // if it doesn't throw test is passed.
    }

    TEST_METHOD(ReadHeaderFromBufferNotStartingWithFFShouldThrow)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0x0F);
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xFF);
        buffer.push_back(0xDA); // SOS: Marks the start of scan.

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::jpeg_marker_start_byte_not_found), error.code().value());
            return;
        }

        Assert::Fail();
    }

    static void ReadHeaderWithApplicationData(uint8_t dataNumber)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8); // SOI: Marks the start of an image.
        buffer.push_back(0xFF);
        buffer.push_back(0xE0 + dataNumber);
        buffer.push_back(0x00);
        buffer.push_back(0x02);
        buffer.push_back(0xFF);
        buffer.push_back(0xDA); // SOS: Marks the start of scan.

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        reader.ReadHeader(); // if it doesn't throw test is passed.
    }

    TEST_METHOD(ReadHeaderWithApplicationData)
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

    TEST_METHOD(ReadHeaderWithJpegLSExtendedFrameShouldThrow)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF9); // SOF_57: Marks the start of a JPEG-LS extended (ISO/IEC 14495-2) encoded frame.

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::encoding_not_supported), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderJpegLSPresetParameterSegment)
    {
        vector<uint8_t> source(100);
        const ByteStreamInfo sourceInfo = FromByteArray(source.data(), source.size());

        charls::JpegStreamWriter writer(sourceInfo);
        writer.WriteStartOfImage();

        const jpegls_pc_parameters presets{1, 2, 3, 4, 5};
        writer.WriteJpegLSPresetParametersSegment(presets);
        writer.WriteStartOfFrameSegment(1, 1, 2, 1);
        writer.WriteStartOfScanSegment(1, 0, interleave_mode::none);

        const ByteStreamInfo destinationInfo = FromByteArray(source.data(), source.size());
        JpegStreamReader reader(destinationInfo);

        reader.ReadHeader();
        const auto& actual = reader.GetCustomPreset();

        Assert::AreEqual(presets.maximum_sample_value, actual.maximum_sample_value);
        Assert::AreEqual(presets.reset_value, actual.reset_value);
        Assert::AreEqual(presets.threshold1, actual.threshold1);
        Assert::AreEqual(presets.threshold2, actual.threshold2);
        Assert::AreEqual(presets.threshold3, actual.threshold3);
    }

    TEST_METHOD(ReadHeaderWithTooSmallJpegLSPresetParameterSegmentShouldThrow)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF8); // LSE: Marks the start of a JPEG-LS preset parameters segment.
        buffer.push_back(0x00);
        buffer.push_back(0x02);
        buffer.push_back(0x01);

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooSmallJpegLSPresetParameterSegmentWithCodingParametersShouldThrow)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF8); // LSE: Marks the start of a JPEG-LS preset parameters segment.
        buffer.push_back(0x00);
        buffer.push_back(0x0A);
        buffer.push_back(0x01);

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooLargeJpegLSPresetParameterSegmentWithCodingParametersShouldThrow)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF8); // LSE: Marks the start of a JPEG-LS preset parameters segment.
        buffer.push_back(0x00);
        buffer.push_back(0x0C);
        buffer.push_back(0x01);

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    static void ReadHeaderWithJpegLSPresetParameterWithExtendedIdShouldThrow(uint8_t id)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF8); // LSE: Marks the start of a JPEG-LS preset parameters segment.
        buffer.push_back(0x00);
        buffer.push_back(0x03);
        buffer.push_back(id);

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::jpegls_preset_extended_parameter_type_not_supported), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithJpegLSPresetParameterWithExtendedIdShouldThrow)
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

    TEST_METHOD(ReadHeaderWithTooSmallSegmentSizeShouldThrow)
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

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooSmallStartOfFrameShouldThrow)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF7); // SOF_55: Marks the start of JPEG-LS extended scan.
        buffer.push_back(0x00);
        buffer.push_back(0x07);

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooSmallStartOfFrameInComponentInfoShouldThrow)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xF7); // SOF_55: Marks the start of JPEG-LS extended scan.
        buffer.push_back(0x00);
        buffer.push_back(0x07);

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooLargeStartOfFrameShouldThrow)
    {
        JpegTestStreamWriter writer;
        writer.WriteStartOfImage();
        writer.WriteStartOfFrameSegment(512, 512, 8, 3);
        writer.data_.push_back(0);
        writer.data_[5]++;
        const ByteStreamInfo byteStream = FromByteArray(writer.data_.data(), writer.data_.size());

        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithDuplicateComponentIdInStartOfFrameSegmentShouldThrow)
    {
        JpegTestStreamWriter writer;
        writer.componentIdOverride = 7;
        writer.WriteStartOfImage();
        writer.WriteStartOfFrameSegment(512, 512, 8, 3);
        const ByteStreamInfo byteStream = FromByteArray(writer.data_.data(), writer.data_.size());

        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::duplicate_component_id_in_sof_segment), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooSmallStartOfScanShouldThrow)
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

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithTooSmallStartOfScanComponentCountShouldThrow)
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

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::invalid_marker_segment_size), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithDirectlyEndOfImageShouldThrow)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xD9); // EOI.

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::unexpected_end_of_image_marker), error.code().value());
            return;
        }

        Assert::Fail();
    }

    TEST_METHOD(ReadHeaderWithDuplicateStartOfImageShouldThrow)
    {
        vector<uint8_t> buffer;
        buffer.push_back(0xFF);
        buffer.push_back(0xD8);
        buffer.push_back(0xFF);
        buffer.push_back(0xD8); // SOI.

        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        try
        {
            reader.ReadHeader();
            Assert::Fail();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::duplicate_start_of_image_marker), error.code().value());
        }
    }

    static void ReadSpiffHeader(uint8_t low_version)
    {
        vector<uint8_t> buffer = create_test_spiff_header(2, low_version);
        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        spiff_header spiff_header{};
        bool spiff_header_found{};

        reader.ReadHeader(&spiff_header, &spiff_header_found);

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

    TEST_METHOD(ReadSpiffHeader)
    {
        ReadSpiffHeader(0);
    }

    TEST_METHOD(read_spiff_header_low_version_newer)
    {
        ReadSpiffHeader(1);
    }

    TEST_METHOD(read_spiff_header_high_version_to_new)
    {
        vector<uint8_t> buffer = create_test_spiff_header(3);
        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        spiff_header spiff_header{};
        bool spiff_header_found{};

        reader.ReadHeader(&spiff_header, &spiff_header_found);

        Assert::IsFalse(spiff_header_found);
    }

    TEST_METHOD(read_spiff_header_without_end_of_directory)
    {
        vector<uint8_t> buffer = create_test_spiff_header(2, 0, false);
        const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
        JpegStreamReader reader(byteStream);

        spiff_header spiff_header{};
        bool spiff_header_found{};

        reader.ReadHeader(&spiff_header, &spiff_header_found);
        Assert::IsTrue(spiff_header_found);

        try
        {
            reader.ReadHeader();
            Assert::Fail();
        }
        catch (const system_error& error)
        {
            Assert::AreEqual(static_cast<int>(jpegls_errc::missing_end_of_spiff_directory), error.code().value());
        }
    }
};

} // namespace CharLSUnitTest
