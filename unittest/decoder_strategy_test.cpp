// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "../src/decoder_strategy.h"

#include "encoder_strategy_tester.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::unique_ptr;

namespace {

class DecoderStrategyTester final : public charls::DecoderStrategy
{
public:
    DecoderStrategyTester(const charls::frame_info& frame_info, const charls::coding_parameters& parameters, uint8_t* destination, const size_t nOutBufLen) :
        DecoderStrategy(frame_info, parameters)
    {
        ByteStreamInfo stream{nullptr, destination, nOutBufLen};
        Init(stream);
    }

    void SetPresets(const charls::jpegls_pc_parameters& /*preset_coding_parameters*/) noexcept(false) override
    {
    }

    unique_ptr<charls::ProcessLine> CreateProcess(ByteStreamInfo /*rawStreamInfo*/, uint32_t /*stride*/) noexcept(false) override
    {
        return nullptr;
    }

    void DecodeScan(unique_ptr<charls::ProcessLine> /*outputData*/, const JlsRect& /*size*/, ByteStreamInfo& /*compressedData*/) noexcept(false) override
    {
    }

    int32_t Read(const int32_t length)
    {
        return ReadLongValue(length);
    }
};

} // namespace


namespace charls {
namespace test {

// clang-format off

TEST_CLASS(DecoderStrategyTest)
{
public:
    TEST_METHOD(DecodeEncodedFFPattern)
    {
        const struct
        {
            int32_t val;
            int bits;
        } inData[5] = { { 0x00, 24 },{ 0xFF, 8 },{ 0xFFFF, 16 },{ 0xFFFF, 16 },{ 0x12345678, 31 } };

        uint8_t encBuf[100];
        const charls::frame_info frame_info{};
        const charls::coding_parameters parameters{};

        EncoderStrategyTester encoder(frame_info, parameters);

        ByteStreamInfo stream;
        stream.rawStream = nullptr;
        stream.rawData = encBuf;
        stream.count = sizeof(encBuf);
        encoder.InitForward(stream);

        for (size_t i = 0; i < sizeof(inData) / sizeof(inData[0]); i++)
        {
            encoder.AppendToBitStreamForward(inData[i].val, inData[i].bits);
        }
        encoder.EndScanForward();
        // Note: Correct encoding is tested in EncoderStrategyTest::AppendToBitStreamFFPattern.

        const auto length = encoder.GetLengthForward();
        DecoderStrategyTester dec(frame_info, parameters, encBuf, length);
        for (auto i = 0U; i < sizeof(inData) / sizeof(inData[0]); i++)
        {
            const auto actual = dec.Read(inData[i].bits);
            Assert::AreEqual(inData[i].val, actual);
        }
    }
};

}
}
