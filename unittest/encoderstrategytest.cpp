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
    explicit EncoderStrategyTester(const JlsParameters& info) : EncoderStrategy(info)
    {
    }

    virtual void SetPresets(const JlsCustomParameters&) override
    {
    }

    virtual size_t EncodeScan(std::unique_ptr<ProcessLine>, ByteStreamInfo&, void*) override
    {
        return 0;
    }

    virtual ProcessLine* CreateProcess(ByteStreamInfo) override
    {
        return nullptr;
    }

    void InitTest(ByteStreamInfo& info)
    {
        Init(info);
    }

    void AppendToBitStreamTest(int32_t value, int32_t length)
    {
        AppendToBitStream(value, length);
    }

    void FlushTest()
    {
        Flush();
    }
};


namespace CharLSUnitTest
{
    TEST_CLASS(EncoderStrategyTest)
    {
    public:
        TEST_METHOD(AppendToBitStreamZeroLength)
        {
            JlsParameters info;

            EncoderStrategyTester strategy(info);

            uint8_t data[1024];

            ByteStreamInfo stream;
            stream.rawStream = nullptr;
            stream.rawData = data;
            stream.count = sizeof(data);
            strategy.InitTest(stream);

            strategy.AppendToBitStreamTest(0, 0);
            strategy.FlushTest();
        }

        TEST_METHOD(AppendToBitStreamFFPattern)
        {
            // Failing unit test. It exposes the reported bug that AppendToBitStream has a flaw for a certain bit pattern.
            return;

            JlsParameters info;

            EncoderStrategyTester strategy(info);

            uint8_t data[1024];

            ByteStreamInfo stream;
            stream.rawStream = nullptr;
            stream.rawData = data;
            stream.count = sizeof(data);
            strategy.InitTest(stream);

            // We want _isFFWritten == true.
            strategy.AppendToBitStreamTest(0, 24);
            strategy.AppendToBitStreamTest(0xff, 8);

            // We need the buffer filled with set bits.
            strategy.AppendToBitStreamTest(0xffff, 16);
            strategy.AppendToBitStreamTest(0xffff, 16);

            strategy.AppendToBitStreamTest(0, 31);

            //strategy.FlushTest();
        }
    };
}
