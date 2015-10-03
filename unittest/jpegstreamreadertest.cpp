//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#include "stdafx.h"

#include "CppUnitTest.h"
#include "..\src\jpegstreamreader.h"
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

            ByteStreamInfo byteStream = FromByteArray(buffer, 0);
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

            ByteStreamInfo byteStream = FromByteArray(&(buffer[0]), 6);
            JpegStreamReader reader(byteStream);

            reader.ReadHeader(); // if it doesn´t throw test is passed.
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

            ByteStreamInfo byteStream = FromByteArray(&(buffer[0]), 6);
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
    };
}