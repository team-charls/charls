// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "../src/encoder_strategy.h"

namespace CharLSUnitTest {

class EncoderStrategyTester final : charls::EncoderStrategy
{
public:
    explicit EncoderStrategyTester(const JlsParameters& params) :
        EncoderStrategy(params)
    {
    }

    void SetPresets(const charls::jpegls_pc_parameters&) noexcept(false) override
    {
    }

    size_t EncodeScan(std::unique_ptr<charls::ProcessLine>, ByteStreamInfo&) noexcept(false) override
    {
        return 0;
    }

    std::unique_ptr<charls::ProcessLine> CreateProcess(ByteStreamInfo) noexcept(false) override
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

    std::size_t GetLengthForward() const noexcept
    {
        return GetLength();
    }

    void EndScanForward()
    {
        EndScan();
    }
};

} // namespace CharLSUnitTest
