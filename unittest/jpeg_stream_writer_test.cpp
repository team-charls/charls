// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include "pch.h"

#include "../src/jpeg_stream_writer.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

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
