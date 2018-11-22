// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include "stdafx.h"

#include "../src/jpeg_stream_writer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CharLSUnitTest
{
    TEST_CLASS(JpegStreamWriterTest)
    {
    public:
        TEST_METHOD(LengthWillbeZeroAfterCreate)
        {
            charls::JpegStreamWriter writer;
            Assert::AreEqual(static_cast<size_t>(0), writer.GetLength());
        }
    };
}
