//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

#include "stdafx.h"

#include "../src/jpegstreamreader.h"
#include <vector>
#include <cstdint>

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace charls;

namespace CharLSUnitTest
{
    TEST_CLASS(JpegStreamReaderTest)
    {
    public:
        TEST_METHOD(ReadHeaderFromToSmallInputBuffer)
        {
            uint8_t buffer[1];

            const ByteStreamInfo byteStream = FromByteArray(buffer, 0);
            JpegStreamReader reader(byteStream);

            try
            {
                reader.ReadHeader();
            }
            catch (const system_error &error)
            {
                Assert::AreEqual(static_cast<int>(ApiResult::CompressedBufferTooSmall), error.code().value());
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
            catch (const system_error &error)
            {
                Assert::AreEqual(static_cast<int>(ApiResult::MissingJpegMarkerStart), error.code().value());
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
            buffer.push_back(0xF9); // SOF_59: Marks the start of JPEG-LS extended scan.

            const ByteStreamInfo byteStream = FromByteArray(buffer.data(), buffer.size());
            JpegStreamReader reader(byteStream);

            try
            {
                reader.ReadHeader();
            }
            catch (const system_error& error)
            {
                Assert::AreEqual(static_cast<int>(ApiResult::UnsupportedEncoding), error.code().value());
                return;
            }

            Assert::Fail();
        }

        static void ReadHeaderWithJpegLSExtendedPresetParameterIdShouldThrow(uint8_t id)
        {
            vector<uint8_t> buffer;
            buffer.push_back(0xFF);
            buffer.push_back(0xD8);
            buffer.push_back(0xFF);
            buffer.push_back(0xF8); // SOF_59: Marks the start of JPEG-LS extended scan.
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
                Assert::AreEqual(static_cast<int>(ApiResult::UnsupportedEncoding), error.code().value());
                return;
            }

            Assert::Fail();
        }

        TEST_METHOD(ReadHeaderWithJpegLSExtendedPresetParameterIdShouldThrow)
        {
            ReadHeaderWithJpegLSExtendedPresetParameterIdShouldThrow(0x5);
            ReadHeaderWithJpegLSExtendedPresetParameterIdShouldThrow(0x6);
            ReadHeaderWithJpegLSExtendedPresetParameterIdShouldThrow(0x7);
            ReadHeaderWithJpegLSExtendedPresetParameterIdShouldThrow(0x8);
            ReadHeaderWithJpegLSExtendedPresetParameterIdShouldThrow(0x9);
            ReadHeaderWithJpegLSExtendedPresetParameterIdShouldThrow(0xA);
            ReadHeaderWithJpegLSExtendedPresetParameterIdShouldThrow(0xC);
            ReadHeaderWithJpegLSExtendedPresetParameterIdShouldThrow(0xD);
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
                Assert::AreEqual(static_cast<int>(ApiResult::InvalidCompressedData), error.code().value());
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
                Assert::AreEqual(static_cast<int>(ApiResult::InvalidCompressedData), error.code().value());
                return;
            }

            Assert::Fail();
        }

    };
}
