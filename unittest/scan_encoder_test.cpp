// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "scan_encoder_tester.hpp"
#include "util.hpp"

#include <array>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;
using std::byte;

namespace charls::test {

TEST_CLASS(scan_encoder_test)
{
public:
    TEST_METHOD(append_to_bit_stream_zero_length) // NOLINT
    {
        constexpr frame_info frame_info{1, 1, 8, 1};
        constexpr coding_parameters parameters{};

        scan_encoder_tester scan_encoder(frame_info, parameters);

        array<byte, 1024> data{};

        scan_encoder.initialize_forward({data.data(), data.size()});

        scan_encoder.append_to_bit_stream_forward(0, 0);
        scan_encoder.flush_forward();
        Assert::AreEqual({}, data[0]);
    }

    TEST_METHOD(append_to_bit_stream_ff_pattern) // NOLINT
    {
        constexpr frame_info frame_info{1, 1, 8, 1};
        constexpr coding_parameters parameters{};

        scan_encoder_tester scan_encoder(frame_info, parameters);

        array<byte, 1024> destination{};
        destination[13] = byte{0x77}; // marker byte to detect overruns.

        scan_encoder.initialize_forward({destination.data(), destination.size()});

        // We want _isFFWritten == true.
        scan_encoder.append_to_bit_stream_forward(0, 24);
        scan_encoder.append_to_bit_stream_forward(0xff, 8);

        // We need the buffer filled with set bits.
        scan_encoder.append_to_bit_stream_forward(0xffff, 16);
        scan_encoder.append_to_bit_stream_forward(0xffff, 16);

        // Buffer is full of FFs and _isFFWritten = true: Flush can only write 30 date bits.
        scan_encoder.append_to_bit_stream_forward(0x3, 31);

        scan_encoder.flush_forward();

        // Verify output.
        Assert::AreEqual(size_t{13}, scan_encoder.get_length_forward());
        Assert::AreEqual({}, destination[0]);
        Assert::AreEqual({}, destination[1]);
        Assert::AreEqual({}, destination[2]);
        Assert::AreEqual(byte{0xFF}, destination[3]);
        Assert::AreEqual(byte{0x7F}, destination[4]); // extra 0 bit.
        Assert::AreEqual(byte{0xFF}, destination[5]);
        Assert::AreEqual(byte{0x7F}, destination[6]); // extra 0 bit.
        Assert::AreEqual(byte{0xFF}, destination[7]);
        Assert::AreEqual(byte{0x60}, destination[8]);
        Assert::AreEqual({}, destination[9]);
        Assert::AreEqual({}, destination[10]);
        Assert::AreEqual({}, destination[11]);
        Assert::AreEqual(byte{0xC0}, destination[12]);
        Assert::AreEqual(byte{0x77}, destination[13]);
    }
};

} // namespace charls::test
