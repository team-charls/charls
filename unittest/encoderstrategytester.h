//
// (C) CharLS Team 2016, all rights reserved. See the accompanying "License.txt" for licensed use. 
//
#pragma once


#include "..\src\encoderstrategy.h"


class EncoderStrategyTester : EncoderStrategy
{
public:
    explicit EncoderStrategyTester(const JlsParameters& params) : EncoderStrategy(params)
    {
    }

    void SetPresets(const JpegLSPresetCodingParameters&) override
    {
    }

    size_t EncodeScan(std::unique_ptr<ProcessLine>, ByteStreamInfo&) override
    {
        return 0;
    }

    std::unique_ptr<ProcessLine> CreateProcess(ByteStreamInfo) override
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

    std::size_t GetLengthForward() const
    {
        return GetLength();
    }

    void EndScanForward()
    {
        EndScan();
    }
};

