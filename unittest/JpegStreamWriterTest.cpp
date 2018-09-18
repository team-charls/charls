// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include "stdafx.h"

#include "..\src\jpegstreamwriter.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CharLSUnitTest
{
    TEST_CLASS(JpegStreamWriterTest)
    {
    public:
        TEST_METHOD(LengthWillbeZeroAfterCreate)
        {
            JpegStreamWriter writer;
            Assert::AreEqual(static_cast<size_t>(0), writer.GetLength());
        }
    };
}
