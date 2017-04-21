//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

#include "stdafx.h"

#include "..\src\colortransform.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CharLSUnitTest
{
    TEST_CLASS(ColorTransformTest)
    {
    public:
        TEST_METHOD(TransformHp3RoundTrip)
        {
            // For the normal unit test keep the range small for a quick test.
            // For a complete test which will take a while set the start and end to 0 and 255.
            const uint8_t startValue = 123;
            const uint8_t endValue = 124;

            TransformHp3<uint8_t> transformation;

            for (uint8_t red = startValue; red < endValue; ++red)
            {
                for (uint8_t green = 0; green < 255; ++green)
                {
                    for (uint8_t blue = 0; blue < 255; ++blue)
                    {
                        auto sample = transformation(red, green, blue);
                        TransformHp3<uint8_t>::INVERSE inverse(transformation);

                        auto roundTrip = inverse(sample.v1, sample.v2, sample.v3);

                        Assert::AreEqual(red, roundTrip.R);
                        Assert::AreEqual(green, roundTrip.G);
                        Assert::AreEqual(blue, roundTrip.B);
                    }
                }
            }
        }
    };
}