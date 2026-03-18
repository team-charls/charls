// SPDX-FileCopyrightText: © 2015 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include <gtest/gtest.h>

#include "scan_encoder_tester.hpp"
#include "support.hpp"

#include <array>

using std::array;
using std::byte;

namespace charls::test {

TEST(scan_encoder_test, append_to_bit_stream_zero_length)
{
    constexpr frame_info frame_info{1, 1, 8, 1};
    constexpr coding_parameters parameters{};

    scan_encoder_tester scan_encoder(frame_info, parameters);

    array<byte, 1024> data{};

    scan_encoder.initialize_forward({data.data(), data.size()});

    scan_encoder.append_to_bit_stream_forward(0, 0);
    scan_encoder.flush_forward();
    EXPECT_EQ(byte{}, data[0]);
}

TEST(scan_encoder_test, append_to_bit_stream_ff_pattern)
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
    EXPECT_EQ(size_t{13}, scan_encoder.get_length_forward());
    EXPECT_EQ(byte{}, destination[0]);
    EXPECT_EQ(byte{}, destination[1]);
    EXPECT_EQ(byte{}, destination[2]);
    EXPECT_EQ(byte{0xFF}, destination[3]);
    EXPECT_EQ(byte{0x7F}, destination[4]); // extra 0 bit.
    EXPECT_EQ(byte{0xFF}, destination[5]);
    EXPECT_EQ(byte{0x7F}, destination[6]); // extra 0 bit.
    EXPECT_EQ(byte{0xFF}, destination[7]);
    EXPECT_EQ(byte{0x60}, destination[8]);
    EXPECT_EQ(byte{}, destination[9]);
    EXPECT_EQ(byte{}, destination[10]);
    EXPECT_EQ(byte{}, destination[11]);
    EXPECT_EQ(byte{0xC0}, destination[12]);
    EXPECT_EQ(byte{0x77}, destination[13]);
}

} // namespace charls::test
