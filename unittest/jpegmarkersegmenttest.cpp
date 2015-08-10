//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#include "stdafx.h"

#include "CppUnitTest.h"
#include "..\src\jpegmarkersegment.h"
#include <memory>
#include <cstdint>

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CharLSUnitTest
{
    TEST_CLASS(JpegMarkerSegmentTest)
    {
    private:
        static size_t SerializeSegment(unique_ptr<JpegMarkerSegment> segment, uint8_t* buffer, size_t count)
        {
            ByteStreamInfo info = FromByteArray(buffer, count);
            JpegStreamWriter writer;
            writer.AddSegment(segment.release());
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

            auto segment = unique_ptr<JpegMarkerSegment>(JpegMarkerSegment::CreateStartOfFrameMarker(100, UINT16_MAX, bitsPerSample, componentCount));

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
			
			auto segment = unique_ptr<JpegMarkerSegment>(JpegMarkerSegment::CreateStartOfFrameMarker(0, 0, bitsPerSample, componentCount));

            uint8_t buffer[17];
            auto bytesWritten = SerializeSegment(move(segment), buffer, _countof(buffer));
            Assert::AreEqual(static_cast<size_t>(17), bytesWritten);
            Assert::AreEqual(static_cast<uint8_t>(bitsPerSample), buffer[6]);
            Assert::AreEqual(static_cast<uint8_t>(componentCount), buffer[11]);
        }

        TEST_METHOD(CreateStartOfFrameMarkerWithHighBoundaryValues)
        {
            auto segment = unique_ptr<JpegMarkerSegment>(JpegMarkerSegment::CreateStartOfFrameMarker(UINT16_MAX, UINT16_MAX, UINT8_MAX, UINT8_MAX - 1));

            uint8_t buffer[776];
            auto bytesWritten = SerializeSegment(move(segment), buffer, _countof(buffer));

            Assert::AreEqual(static_cast<size_t>(776), bytesWritten);
            Assert::AreEqual(static_cast<uint8_t>(UINT8_MAX), buffer[6]);
            Assert::AreEqual(static_cast<uint8_t>(UINT8_MAX - 1), buffer[11]);
        }

        TEST_METHOD(CreateJpegFileInterchangeFormatMarker)
        {
            // TODO
            //static JpegMarkerSegment* CreateJpegFileInterchangeFormatMarker(const JfifParameters& jfif);
        }

        TEST_METHOD(CreateJpegLSExtendedParametersMarker)
        {
            // TODO
            //static JpegMarkerSegment* CreateJpegLSExtendedParametersMarker(const JlsCustomParameters& pcustom);
        }

        TEST_METHOD(CreateColorTransformMarker)
        {
            // TODO
            //static JpegMarkerSegment* CreateColorTransformMarker(int i);
        }

        TEST_METHOD(CreateStartOfScanMarker)
        {
            // TODO
            //static JpegMarkerSegment* CreateStartOfScanMarker(const JlsParameters* pparams, int32_t icomponent);
        }
    };
}