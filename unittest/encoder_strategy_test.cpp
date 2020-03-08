// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "encoder_strategy_tester.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace charls {
namespace test {

// clang-format off

TEST_CLASS(EncoderStrategyTest)
{
public:
    TEST_METHOD(AppendToBitStreamZeroLength)
    {
        const charls::frame_info frame_info{};
        const charls::coding_parameters parameters{};

        EncoderStrategyTester strategy(frame_info, parameters);

        uint8_t data[1024];

        ByteStreamInfo stream;
        stream.rawStream = nullptr;
        stream.rawData = data;
        stream.count = sizeof(data);
        strategy.InitForward(stream);

        strategy.AppendToBitStreamForward(0, 0);
        strategy.FlushForward();
    }

    TEST_METHOD(AppendToBitStreamFFPattern)
    {
        const charls::frame_info frame_info{};
        const charls::coding_parameters parameters{};

        EncoderStrategyTester strategy(frame_info, parameters);

        uint8_t data[1024];
        data[13] = 0x77; // marker byte to detect overruns.

        ByteStreamInfo stream;
        stream.rawStream = nullptr;
        stream.rawData = data;
        stream.count = sizeof(data);
        strategy.InitForward(stream);

        // We want _isFFWritten == true.
        strategy.AppendToBitStreamForward(0, 24);
        strategy.AppendToBitStreamForward(0xff, 8);

        // We need the buffer filled with set bits.
        strategy.AppendToBitStreamForward(0xffff, 16);
        strategy.AppendToBitStreamForward(0xffff, 16);

        // Buffer is full with FFs and _isFFWritten = true: Flush can only write 30 date bits.
        strategy.AppendToBitStreamForward(0x3, 31);

        strategy.FlushForward();

        // Verify output.
        Assert::AreEqual(static_cast<size_t>(13), strategy.GetLengthForward());
        Assert::AreEqual(static_cast<uint8_t>(0x00), data[0]);
        Assert::AreEqual(static_cast<uint8_t>(0x00), data[1]);
        Assert::AreEqual(static_cast<uint8_t>(0x00), data[2]);
        Assert::AreEqual(static_cast<uint8_t>(0xFF), data[3]);
        Assert::AreEqual(static_cast<uint8_t>(0x7F), data[4]); // extra 0 bit.
        Assert::AreEqual(static_cast<uint8_t>(0xFF), data[5]);
        Assert::AreEqual(static_cast<uint8_t>(0x7F), data[6]); // extra 0 bit.
        Assert::AreEqual(static_cast<uint8_t>(0xFF), data[7]);
        Assert::AreEqual(static_cast<uint8_t>(0x60), data[8]);
        Assert::AreEqual(static_cast<uint8_t>(0x00), data[9]);
        Assert::AreEqual(static_cast<uint8_t>(0x00), data[10]);
        Assert::AreEqual(static_cast<uint8_t>(0x00), data[11]);
        Assert::AreEqual(static_cast<uint8_t>(0xC0), data[12]);
        Assert::AreEqual(static_cast<uint8_t>(0x77), data[13]);
    }
};

}
}
