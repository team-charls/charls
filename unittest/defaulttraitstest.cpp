//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#include "stdafx.h"

#include "CppUnitTest.h"
#include "..\src\defaulttraits.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CharLSUnitTest
{
    TEST_CLASS(DefaultTraitsTest)
    {
    public:
        TEST_METHOD(Create)
        {
            DefaultTraitsT<uint8_t, uint8_t> traits((1 << 8) - 1, 0);

            Assert::AreEqual(255, traits.MAXVAL);
            Assert::AreEqual(256, traits.RANGE);
            Assert::AreEqual(0, traits.NEAR);
            Assert::AreEqual(8, traits.qbpp);
            Assert::AreEqual(8, traits.bpp);
            Assert::AreEqual(32, traits.LIMIT);
            Assert::AreEqual(64, traits.RESET);
        }
    };
}
