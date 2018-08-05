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

            const ByteStreamInfo byteStream = FromByteArray(&(buffer[0]), 6);
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

            const ByteStreamInfo byteStream = FromByteArray(&(buffer[0]), 6);
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
    };
}
