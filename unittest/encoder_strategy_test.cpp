// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "encoder_strategy_tester.h"

#include <array>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;

namespace charls { namespace test {

TEST_CLASS(encoder_strategy_test)
{
public:
    TEST_METHOD(append_to_bit_stream_zero_length) // NOLINT
    {
        const frame_info frame_info{};
        const coding_parameters parameters{};

        encoder_strategy_tester strategy(frame_info, parameters);

        array<uint8_t, 1024> data{};

        strategy.initialize_forward({data.data(), data.size()});

        strategy.append_to_bit_stream_forward(0, 0);
        strategy.flush_forward();
    }

    TEST_METHOD(append_to_bit_stream_ff_pattern) // NOLINT
    {
        const frame_info frame_info{};
        const coding_parameters parameters{};

        encoder_strategy_tester strategy(frame_info, parameters);

        array<uint8_t, 1024> destination{};
        destination[13] = 0x77; // marker byte to detect overruns.

        strategy.initialize_forward({destination.data(), destination.size()});

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
        Assert::AreEqual(static_cast<uint8_t>(0x00), destination[0]);
        Assert::AreEqual(static_cast<uint8_t>(0x00), destination[1]);
        Assert::AreEqual(static_cast<uint8_t>(0x00), destination[2]);
        Assert::AreEqual(static_cast<uint8_t>(0xFF), destination[3]);
        Assert::AreEqual(static_cast<uint8_t>(0x7F), destination[4]); // extra 0 bit.
        Assert::AreEqual(static_cast<uint8_t>(0xFF), destination[5]);
        Assert::AreEqual(static_cast<uint8_t>(0x7F), destination[6]); // extra 0 bit.
        Assert::AreEqual(static_cast<uint8_t>(0xFF), destination[7]);
        Assert::AreEqual(static_cast<uint8_t>(0x60), destination[8]);
        Assert::AreEqual(static_cast<uint8_t>(0x00), destination[9]);
        Assert::AreEqual(static_cast<uint8_t>(0x00), destination[10]);
        Assert::AreEqual(static_cast<uint8_t>(0x00), destination[11]);
        Assert::AreEqual(static_cast<uint8_t>(0xC0), destination[12]);
        Assert::AreEqual(static_cast<uint8_t>(0x77), destination[13]);
    }
};

}} // namespace charls::test
