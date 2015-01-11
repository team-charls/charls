//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#include "stdafx.h"

#include "CppUnitTest.h"
#include "..\src\encoderstrategy.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


class EncoderStrategyTester : EncoderStrategy
{
public:
    EncoderStrategyTester(const JlsParameters& info) : EncoderStrategy(info)
    {
    }

    virtual void SetPresets(const JlsCustomParameters&)
    {
    }

    virtual size_t EncodeScan(std::unique_ptr<ProcessLine>, ByteStreamInfo&, void*)
    {
        return 0;
    }

    virtual ProcessLine* CreateProcess(ByteStreamInfo)
    {
        return nullptr;
    }

    void AppendToBitStreamTest(int32_t value, int32_t length)
    {
        AppendToBitStream(value, length);
    }
};


namespace CharLSUnitTest
{
    TEST_CLASS(EncoderStrategyTest)
    {
    public:
        TEST_METHOD(AppendToBitStream)
        {
            JlsParameters info;

            EncoderStrategyTester es(info);

            es.AppendToBitStreamTest(0, 0);
        }
    };
}
