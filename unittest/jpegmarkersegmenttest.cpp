//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#include "stdafx.h"

#include "CppUnitTest.h"
#include "..\src\jpegmarkersegment.h"
#include <memory>
#include <cstdint>

using namespace std;
using namespace charls;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CharLSUnitTest
{
    TEST_CLASS(JpegMarkerSegmentTest)
    {
        static size_t SerializeSegment(unique_ptr<JpegSegment> segment, uint8_t* buffer, size_t count)
        {
            ByteStreamInfo info = FromByteArray(buffer, count);
            JpegStreamWriter writer;
            writer.AddSegment(move(segment));
            auto bytesWritten = writer.Write(info);

            Assert::IsTrue(bytesWritten >= 4);

            // write.Write will always serialize a complete byte stream. Check the leading and trailing JPEG Markers SOI and EOI. 
            Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[0]);
            Assert::AreEqual(static_cast<uint8_t>(0xD8), buffer[1]); // JPEG_SOI

            Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[bytesWritten - 2]);
            Assert::AreEqual(static_cast<uint8_t>(0xD9), buffer[bytesWritten - 1]); // JPEG_EOI

            return bytesWritten;
        }

    public:
        TEST_METHOD(CreateStartOfFrameMarkerAndSerialize)
        {
            int32_t bitsPerSample = 8;
            int32_t componentCount = 3;

            auto segment = JpegMarkerSegment::CreateStartOfFrameSegment(100, UINT16_MAX, bitsPerSample, componentCount);

            uint8_t buffer[23];
            auto bytesWritten = SerializeSegment(move(segment), buffer, _countof(buffer));

            Assert::AreEqual(static_cast<size_t>(23), bytesWritten);

            Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[2]);
            Assert::AreEqual(static_cast<uint8_t>(0xF7), buffer[3]); // JPEG_SOF_55
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[4]);   // 6 + (3 * 3) + 2 (in little endian)
            Assert::AreEqual(static_cast<uint8_t>(17), buffer[5]);  // 6 + (3 * 3) + 2 (in little endian)
            Assert::AreEqual(static_cast<uint8_t>(bitsPerSample), buffer[6]);
            Assert::AreEqual(static_cast<uint8_t>(255), buffer[7]);    // height (in little endian)
            Assert::AreEqual(static_cast<uint8_t>(255), buffer[8]);  // height (in little endian)
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[9]);    // width (in little endian)
            Assert::AreEqual(static_cast<uint8_t>(100), buffer[10]);  // width (in little endian)
            Assert::AreEqual(static_cast<uint8_t>(componentCount), buffer[11]);

            Assert::AreEqual(static_cast<uint8_t>(1), buffer[12]);
            Assert::AreEqual(static_cast<uint8_t>(0x11), buffer[13]);
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[14]);

            Assert::AreEqual(static_cast<uint8_t>(2), buffer[15]);
            Assert::AreEqual(static_cast<uint8_t>(0x11), buffer[16]);
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[17]);

            Assert::AreEqual(static_cast<uint8_t>(3), buffer[18]);
            Assert::AreEqual(static_cast<uint8_t>(0x11), buffer[19]);
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[20]);
        }

        TEST_METHOD(CreateStartOfFrameMarkerWithLowBoundaryValuesAndSerialize)
        {
            const int32_t bitsPerSample = 2;
            const int32_t componentCount = 1;
            
            auto segment = JpegMarkerSegment::CreateStartOfFrameSegment(0, 0, bitsPerSample, componentCount);

            uint8_t buffer[17];
            auto bytesWritten = SerializeSegment(move(segment), buffer, _countof(buffer));
            Assert::AreEqual(static_cast<size_t>(17), bytesWritten);
            Assert::AreEqual(static_cast<uint8_t>(bitsPerSample), buffer[6]);
            Assert::AreEqual(static_cast<uint8_t>(componentCount), buffer[11]);
        }

        TEST_METHOD(CreateStartOfFrameMarkerWithHighBoundaryValuesAndSerialize)
        {
            auto segment = JpegMarkerSegment::CreateStartOfFrameSegment(UINT16_MAX, UINT16_MAX, UINT8_MAX, UINT8_MAX - 1);

            uint8_t buffer[776];
            auto bytesWritten = SerializeSegment(move(segment), buffer, _countof(buffer));

            Assert::AreEqual(static_cast<size_t>(776), bytesWritten);
            Assert::AreEqual(static_cast<uint8_t>(UINT8_MAX), buffer[6]);
            Assert::AreEqual(static_cast<uint8_t>(UINT8_MAX - 1), buffer[11]);
        }

        TEST_METHOD(CreateJpegFileInterchangeFormatMarkerAndSerialize)
        {
            JfifParameters params;

            params.version = (1 * 256) + 2;
            params.units = 2;
            params.Xdensity = 96;
            params.Ydensity = 300;
            params.Xthumbnail = 0;
            params.Ythumbnail = 0;

            auto segment = JpegMarkerSegment::CreateJpegFileInterchangeFormatSegment(params);

            uint8_t buffer[22];
            auto bytesWritten = SerializeSegment(move(segment), buffer, _countof(buffer));

            Assert::AreEqual(static_cast<size_t>(22), bytesWritten);

            // Verify JFIF identifier string.
            Assert::AreEqual(static_cast<uint8_t>(0x4A), buffer[6]);
            Assert::AreEqual(static_cast<uint8_t>(0x46), buffer[7]);
            Assert::AreEqual(static_cast<uint8_t>(0x49), buffer[8]);
            Assert::AreEqual(static_cast<uint8_t>(0x46), buffer[9]);
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[10]);

            // Verify version
            Assert::AreEqual(static_cast<uint8_t>(1), buffer[11]);
            Assert::AreEqual(static_cast<uint8_t>(2), buffer[12]);

            Assert::AreEqual(static_cast<uint8_t>(params.units), buffer[13]);

            // Xdensity
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[14]);
            Assert::AreEqual(static_cast<uint8_t>(96), buffer[15]);

            // Ydensity
            Assert::AreEqual(static_cast<uint8_t>(1), buffer[16]);
            Assert::AreEqual(static_cast<uint8_t>(44), buffer[17]);
        }

        TEST_METHOD(CreateJpegLSExtendedParametersMarkerAndSerialize)
        {
            JlsCustomParameters params;

            params.MAXVAL = 2;
            params.T1 = 1;
            params.T2 = 2;
            params.T3 = 3;
            params.RESET = 7;

            auto segment = JpegMarkerSegment::CreateJpegLSExtendedParametersSegment(params);

            uint8_t buffer[19];
            auto bytesWritten = SerializeSegment(move(segment), buffer, _countof(buffer));
            Assert::AreEqual(static_cast<size_t>(19), bytesWritten);

            // Parameter ID.
            Assert::AreEqual(static_cast<uint8_t>(0x1), buffer[6]);

            // MAXVAL
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[7]);
            Assert::AreEqual(static_cast<uint8_t>(2), buffer[8]);

            // T1 
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[9]);
            Assert::AreEqual(static_cast<uint8_t>(1), buffer[10]);

            // T2
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[11]);
            Assert::AreEqual(static_cast<uint8_t>(2), buffer[12]);

            // T3
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[13]);
            Assert::AreEqual(static_cast<uint8_t>(3), buffer[14]);

            // RESET
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[15]);
            Assert::AreEqual(static_cast<uint8_t>(7), buffer[16]);
        }

        TEST_METHOD(CreateColorTransformMarkerAndSerialize)
        {
            ColorTransformation transformation = ColorTransformation::HP1;

            auto segment = JpegMarkerSegment::CreateColorTransformSegment(transformation);

            uint8_t buffer[13];
            auto bytesWritten = SerializeSegment(move(segment), buffer, _countof(buffer));
            Assert::AreEqual(static_cast<size_t>(13), bytesWritten);

            // Verify mrfx identifier string.
            Assert::AreEqual(static_cast<uint8_t>('m'), buffer[6]);
            Assert::AreEqual(static_cast<uint8_t>('r'), buffer[7]);
            Assert::AreEqual(static_cast<uint8_t>('f'), buffer[8]);
            Assert::AreEqual(static_cast<uint8_t>('x'), buffer[9]);

            Assert::AreEqual(static_cast<uint8_t>(transformation), buffer[10]);
        }

        TEST_METHOD(CreateStartOfScanMarkerAndSerialize)
        {
            auto segment = JpegMarkerSegment::CreateStartOfScanSegment(6, 1, 2, InterleaveMode::None);

            uint8_t buffer[14];
            auto bytesWritten = SerializeSegment(move(segment), buffer, _countof(buffer));
            Assert::AreEqual(static_cast<size_t>(14), bytesWritten);

            Assert::AreEqual(static_cast<uint8_t>(1), buffer[6]); // component count.
            Assert::AreEqual(static_cast<uint8_t>(6), buffer[7]); // component index.
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[8]); // table ID.
            Assert::AreEqual(static_cast<uint8_t>(2), buffer[9]); // NEAR parameter.
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[10]); // ILV parameter.
            Assert::AreEqual(static_cast<uint8_t>(0), buffer[11]); // transformation.
        }
    };
}