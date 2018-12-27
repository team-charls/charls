// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include "pch.h"

#include "../src/jpeg_stream_writer.h"
#include "../src/jpeg_marker_code.h"

#include <array>

using std::array;
using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using charls::JpegStreamWriter;
using charls::JpegMarkerCode;
using charls::ColorTransformation;
using charls::InterleaveMode;

namespace CharLSUnitTest
{
    TEST_CLASS(JpegStreamWriterTest)
    {
    public:
        TEST_METHOD(LengthWillbeZeroAfterCreate)
        {
            const JpegStreamWriter writer;
            Assert::AreEqual(static_cast<size_t>(0), writer.GetLength());
        }

        TEST_METHOD(WriteStartOfImage)
        {
            array<uint8_t, 2> buffer{};
            const ByteStreamInfo info = FromByteArray(buffer.data(), buffer.size());

            JpegStreamWriter writer(info);

            writer.WriteStartOfImage();

            Assert::AreEqual(static_cast<size_t>(2), writer.GetBytesWritten());
            Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[0]);
            Assert::AreEqual(static_cast<uint8_t>(JpegMarkerCode::StartOfImage), buffer[1]);
        }

        TEST_METHOD(WriteEndOfImage)
        {
            array<uint8_t, 2> buffer{};
            const ByteStreamInfo info = FromByteArray(buffer.data(), buffer.size());

            JpegStreamWriter writer(info);

            writer.WriteEndOfImage();

            Assert::AreEqual(static_cast<size_t>(2), writer.GetBytesWritten());
            Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[0]);
            Assert::AreEqual(static_cast<uint8_t>(JpegMarkerCode::EndOfImage), buffer[1]);
        }

        TEST_METHOD(WriteJpegFileInterchangeFormatSegment)
        {
            array<uint8_t, 18> buffer{};
            const ByteStreamInfo info = FromByteArray(buffer.data(), buffer.size());

            JpegStreamWriter writer(info);

            JfifParameters params{};
            params.version = (1 * 256) + 2;
            params.units = 2;
            params.Xdensity = 96;
            params.Ydensity = 300;
            params.Xthumbnail = 0;
            params.Ythumbnail = 0;

            writer.WriteJpegFileInterchangeFormatSegment(params);

            Assert::AreEqual(static_cast<size_t>(18), writer.GetBytesWritten());

            Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[0]);
            Assert::AreEqual(static_cast<uint8_t>(JpegMarkerCode::ApplicationData0), buffer[1]);

            Assert::AreEqual(static_cast<uint8_t>(0), buffer[2]);
            Assert::AreEqual(static_cast<uint8_t>(16), buffer[3]);

            // Verify JFIF identifier string.
            Assert::AreEqual(static_cast<uint8_t>(0x4A), buffer[4]);
            Assert::AreEqual(static_cast<uint8_t>(0x46), buffer[5]);
            Assert::AreEqual(static_cast<uint8_t>(0x49), buffer[6]);
            Assert::AreEqual(static_cast<uint8_t>(0x46), buffer[7]);
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[8]);

            // Verify version
            Assert::AreEqual(static_cast<uint8_t>(1), buffer[9]);
            Assert::AreEqual(static_cast<uint8_t>(2), buffer[10]);

            Assert::AreEqual(static_cast<uint8_t>(params.units), buffer[11]);

            // Xdensity
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[12]);
            Assert::AreEqual(static_cast<uint8_t>(96), buffer[13]);

            // Ydensity
            Assert::AreEqual(static_cast<uint8_t>(1), buffer[14]);
            Assert::AreEqual(static_cast<uint8_t>(44), buffer[15]);
        }

        TEST_METHOD(WriteStartOfFrameSegment)
        {
            const int32_t bitsPerSample = 8;
            const int32_t componentCount = 3;

            array<uint8_t, 19> buffer{};
            const ByteStreamInfo info = FromByteArray(buffer.data(), buffer.size());
            JpegStreamWriter writer(info);

            writer.WriteStartOfFrameSegment(100, UINT16_MAX, bitsPerSample, componentCount);

            Assert::AreEqual(static_cast<size_t>(19), writer.GetBytesWritten());

            Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[0]);
            Assert::AreEqual(static_cast<uint8_t>(0xF7), buffer[1]); // JPEG_SOF_55
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[2]);    // 6 + (3 * 3) + 2 (in little endian)
            Assert::AreEqual(static_cast<uint8_t>(17), buffer[3]);   // 6 + (3 * 3) + 2 (in little endian)
            Assert::AreEqual(static_cast<uint8_t>(bitsPerSample), buffer[4]);
            Assert::AreEqual(static_cast<uint8_t>(255), buffer[5]);  // height (in little endian)
            Assert::AreEqual(static_cast<uint8_t>(255), buffer[6]);  // height (in little endian)
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[7]);    // width (in little endian)
            Assert::AreEqual(static_cast<uint8_t>(100), buffer[8]); // width (in little endian)
            Assert::AreEqual(static_cast<uint8_t>(componentCount), buffer[9]);

            Assert::AreEqual(static_cast<uint8_t>(1), buffer[10]);
            Assert::AreEqual(static_cast<uint8_t>(0x11), buffer[11]);
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[12]);

            Assert::AreEqual(static_cast<uint8_t>(2), buffer[13]);
            Assert::AreEqual(static_cast<uint8_t>(0x11), buffer[14]);
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[15]);

            Assert::AreEqual(static_cast<uint8_t>(3), buffer[16]);
            Assert::AreEqual(static_cast<uint8_t>(0x11), buffer[17]);
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[18]);
        }

        TEST_METHOD(WriteStartOfFrameMarkerSegmentWithLowBoundaryValues)
        {
            const int32_t bitsPerSample = 2;
            const int32_t componentCount = 1;

            array<uint8_t, 13> buffer{};
            const ByteStreamInfo info = FromByteArray(buffer.data(), buffer.size());
            JpegStreamWriter writer(info);

            writer.WriteStartOfFrameSegment(0, 0, bitsPerSample, componentCount);

            Assert::AreEqual(buffer.size(), writer.GetBytesWritten());
            Assert::AreEqual(static_cast<uint8_t>(bitsPerSample), buffer[4]);
            Assert::AreEqual(static_cast<uint8_t>(componentCount), buffer[9]);
        }

        TEST_METHOD(WriteStartOfFrameMarkerSegmentWithHighBoundaryValuesAndSerialize)
        {
            array<uint8_t, 775> buffer{};
            const ByteStreamInfo info = FromByteArray(buffer.data(), buffer.size());
            JpegStreamWriter writer(info);

            writer.WriteStartOfFrameSegment(UINT16_MAX, UINT16_MAX, UINT8_MAX, UINT8_MAX);

            Assert::AreEqual(buffer.size(), writer.GetBytesWritten());
            Assert::AreEqual(static_cast<uint8_t>(UINT8_MAX), buffer[4]);
            Assert::AreEqual(static_cast<uint8_t>(UINT8_MAX), buffer[9]);
        }

        TEST_METHOD(WriteColorTransformSegment)
        {
            const ColorTransformation transformation = ColorTransformation::HP1;

            array<uint8_t, 9> buffer{};
            const ByteStreamInfo info = FromByteArray(buffer.data(), buffer.size());
            JpegStreamWriter writer(info);

            writer.WriteColorTransformSegment(transformation);
            Assert::AreEqual(buffer.size(), writer.GetBytesWritten());

            // Verify mrfx identifier string.
            Assert::AreEqual(static_cast<uint8_t>('m'), buffer[4]);
            Assert::AreEqual(static_cast<uint8_t>('r'), buffer[5]);
            Assert::AreEqual(static_cast<uint8_t>('f'), buffer[6]);
            Assert::AreEqual(static_cast<uint8_t>('x'), buffer[7]);

            Assert::AreEqual(static_cast<uint8_t>(transformation), buffer[8]);
        }

        TEST_METHOD(WriteJpegLSExtendedParametersMarkerAndSerialize)
        {
            JpegLSPresetCodingParameters params{};

            params.MaximumSampleValue = 2;
            params.Threshold1 = 1;
            params.Threshold2 = 2;
            params.Threshold3 = 3;
            params.ResetValue = 7;

            array<uint8_t, 15> buffer{};
            const ByteStreamInfo info = FromByteArray(buffer.data(), buffer.size());
            JpegStreamWriter writer(info);

            writer.WriteJpegLSPresetParametersSegment(params);
            Assert::AreEqual(buffer.size(), writer.GetBytesWritten());

            // Parameter ID.
            Assert::AreEqual(static_cast<uint8_t>(0x1), buffer[4]);

            // MaximumSampleValue
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[5]);
            Assert::AreEqual(static_cast<uint8_t>(2), buffer[6]);

            // Threshold1
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[7]);
            Assert::AreEqual(static_cast<uint8_t>(1), buffer[8]);

            // Threshold2
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[9]);
            Assert::AreEqual(static_cast<uint8_t>(2), buffer[10]);

            // Threshold3
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[11]);
            Assert::AreEqual(static_cast<uint8_t>(3), buffer[12]);

            // ResetValue
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[13]);
            Assert::AreEqual(static_cast<uint8_t>(7), buffer[14]);
        }

        TEST_METHOD(WriteStartOfScanMarker)
        {
            array<uint8_t, 10> buffer{};
            const ByteStreamInfo info = FromByteArray(buffer.data(), buffer.size());
            JpegStreamWriter writer(info);

            writer.WriteStartOfScanSegment(1, 2, InterleaveMode::None);

            Assert::AreEqual(buffer.size(), writer.GetBytesWritten());
            Assert::AreEqual(static_cast<uint8_t>(1), buffer[4]);  // component count.
            Assert::AreEqual(static_cast<uint8_t>(1), buffer[5]);  // component index.
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[6]);  // table ID.
            Assert::AreEqual(static_cast<uint8_t>(2), buffer[7]);  // NEAR parameter.
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[8]); // ILV parameter.
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[9]); // transformation.
        }
    };
}
