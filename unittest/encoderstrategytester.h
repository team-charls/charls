//
// (C) CharLS Team 2016, all rights reserved. See the accompanying "License.txt" for licensed use. 
//
#pragma once


#include "..\src\encoderstrategy.h"


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

    void InitForward(ByteStreamInfo& info)
    {
        Init(info);
    }

    void AppendToBitStreamForward(int32_t value, int32_t length)
    {
        AppendToBitStream(value, length);
    }

    void FlushForward()
    {
        Flush();
    }

    std::size_t GetLengthForward()
    {
        return GetLength();
    }

    void EndScanForward()
    {
        EndScan();
    }
};

