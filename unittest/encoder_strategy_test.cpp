// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "encoder_strategy_tester.h"

#include <array>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;

namespace charls {
namespace test {

// clang-format off

TEST_CLASS(encoder_strategy_test)
{
public:
    TEST_METHOD(append_to_bit_stream_zero_length) // NOLINT
    {
        const frame_info frame_info{};
        const coding_parameters parameters{};

        encoder_strategy_tester strategy(frame_info, parameters);

        array<uint8_t, 1024> data{};

        byte_span stream{data.data(), data.size()};
        strategy.initialize_forward(stream);

        strategy.append_to_bit_stream_forward(0, 0);
        strategy.flush_forward();
    }

    TEST_METHOD(append_to_bit_stream_ff_pattern) // NOLINT
    {
        const frame_info frame_info{};
        const coding_parameters parameters{};

        encoder_strategy_tester strategy(frame_info, parameters);

        array<uint8_t, 1024> data{};
        data[13] = 0x77; // marker byte to detect overruns.

        byte_span stream{data.data(), data.size()};
        strategy.initialize_forward(stream);

        // We want _isFFWritten == true.
        strategy.append_to_bit_stream_forward(0, 24);
        strategy.append_to_bit_stream_forward(0xff, 8);

        // We need the buffer filled with set bits.
        strategy.append_to_bit_stream_forward(0xffff, 16);
        strategy.append_to_bit_stream_forward(0xffff, 16);

        // Buffer is full with FFs and _isFFWritten = true: Flush can only write 30 date bits.
        strategy.append_to_bit_stream_forward(0x3, 31);

        strategy.flush_forward();

        // Verify output.
        Assert::AreEqual(static_cast<size_t>(13), strategy.get_length_forward());
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

} // namespace test
} // namespace charls
