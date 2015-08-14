//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#include "stdafx.h"

#include "CppUnitTest.h"
#include "..\src\jpegstreamreader.h"
#include <cstdint>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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
            catch (const std::system_error &error)
            {
                Assert::AreEqual(error.code().value(), static_cast<int>(charls::ApiResult::CompressedBufferTooSmall));
                return;
            }

            Assert::Fail();
        }
    };
}