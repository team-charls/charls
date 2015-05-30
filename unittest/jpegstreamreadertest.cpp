//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#include "stdafx.h"

#include "CppUnitTest.h"
#include "..\src\jpegstreamreader.h"
#include <cstdint>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// Verify that a function raises an exception:
template<typename _EXPECTEDEXCEPTION, typename _FUNCTOR> static _EXPECTEDEXCEPTION Assert_ExpectException(_FUNCTOR functor, const wchar_t* message = NULL, const __LineInfo* pLineInfo = NULL)
{
    try
    {
        functor();
    }
    catch (_EXPECTEDEXCEPTION const &error)
    {
        return error;
    }
    catch (...)
    {
        Assert::Internal_SetExpectedExceptionMessage(reinterpret_cast<const unsigned short *>(message));
        throw;
    }

    Assert::Fail(message, pLineInfo);
    return _EXPECTEDEXCEPTION();
}

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