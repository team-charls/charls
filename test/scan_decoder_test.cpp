// SPDX-FileCopyrightText: © 2016 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "support.hpp"

#include "../src/scan_decoder.hpp"

#include "scan_encoder_tester.hpp"

#include <array>

using std::array;
using std::byte;

namespace charls::test {

namespace {

class scan_decoder_tester final : public scan_decoder
{
public:
    scan_decoder_tester(const charls::frame_info& frame_info, const coding_parameters& parameters, byte* const destination,
                        const size_t count) :
        scan_decoder(frame_info, {}, parameters)
    {
        initialize({destination, count});
    }

    size_t decode_scan(span<const byte> /*source*/, byte* /*destination*/, size_t /*stride*/) noexcept(false) override
    {
        return {};
    }

    [[nodiscard]]
    int32_t read(const int32_t length)
    {
        return read_long_value(length);
    }

    [[nodiscard]]
    size_t peek_byte_forward()
    {
        return peek_byte();
    }

    [[nodiscard]]
    uintptr_t read_bit_forward()
    {
        return read_bit();
    }

    [[nodiscard]]
    int32_t peek_0_bits_forward()
    {
        return peek_0_bits();
    }
};

} // namespace


TEST(scan_decoder_test, decode_encoded_ff_pattern)
{
    struct data_t final
    {
        uint32_t value;
        int bits;
    };

    constexpr array<data_t, 5> in_data{{{0x00, 24}, {0xFF, 8}, {0xFFFF, 16}, {0xFFFF, 16}, {0x12345678, 31}}};

    array<byte, 100> enc_buf{};
    constexpr frame_info frame_info{1, 1, 8, 1};
    constexpr coding_parameters parameters{};

    scan_encoder_tester scan_encoder(frame_info, parameters);

    scan_encoder.initialize_forward({enc_buf.data(), enc_buf.size()});

    for (const auto& [value, bits] : in_data)
    {
        scan_encoder.append_to_bit_stream_forward(value, bits);
    }

    scan_encoder.end_scan_forward();
    // Note: Correct encoding is tested in scan_encoder_test::append_to_bit_stream_ff_pattern.

    const auto length{scan_encoder.get_length_forward()};
    scan_decoder_tester scan_decoder(frame_info, parameters, enc_buf.data(), length);
    for (const auto& [value, bits] : in_data)
    {
        const auto actual{static_cast<uint32_t>(scan_decoder.read(bits))};
        EXPECT_EQ(value, actual);
    }
}

TEST(scan_decoder_test, peek_byte)
{
    constexpr frame_info frame_info{1, 1, 8, 1};
    constexpr coding_parameters parameters{};

    array buffer{byte{7}, byte{100}, byte{23}, byte{99}};

    scan_decoder_tester scan_decoder(frame_info, parameters, buffer.data(), buffer.size());

    EXPECT_EQ(size_t{7}, scan_decoder.peek_byte_forward());
}

TEST(scan_decoder_test, read_bit)
{
    constexpr frame_info frame_info{1, 1, 8, 1};
    constexpr coding_parameters parameters{};

    array buffer{byte{0xAA}, byte{100}, byte{23}, byte{99}};

    scan_decoder_tester scan_decoder(frame_info, parameters, buffer.data(), buffer.size());

    EXPECT_EQ(static_cast<uintptr_t>(1), scan_decoder.read_bit_forward());
    EXPECT_EQ(static_cast<uintptr_t>(0), scan_decoder.read_bit_forward());
    EXPECT_EQ(static_cast<uintptr_t>(1), scan_decoder.read_bit_forward());
    EXPECT_EQ(static_cast<uintptr_t>(0), scan_decoder.read_bit_forward());
    EXPECT_EQ(static_cast<uintptr_t>(1), scan_decoder.read_bit_forward());
    EXPECT_EQ(static_cast<uintptr_t>(0), scan_decoder.read_bit_forward());
    EXPECT_EQ(static_cast<uintptr_t>(1), scan_decoder.read_bit_forward());
    EXPECT_EQ(static_cast<uintptr_t>(0), scan_decoder.read_bit_forward());
}

TEST(scan_decoder_test, peek_0_bits)
{
    constexpr frame_info frame_info{1, 1, 8, 1};
    constexpr coding_parameters parameters{};

    {
        array buffer{byte{0xF}, byte{100}, byte{23}, byte{99}};

        scan_decoder_tester scan_decoder(frame_info, parameters, buffer.data(), buffer.size());
        EXPECT_EQ(4, scan_decoder.peek_0_bits_forward());
    }

    {
        array buffer{byte{0}, byte{1}, byte{0}, byte{0}};

        scan_decoder_tester scan_decoder(frame_info, parameters, buffer.data(), buffer.size());
        EXPECT_EQ(15, scan_decoder.peek_0_bits_forward());
    }
}

TEST(scan_decoder_test, peek_0_bits_empty_buffer)
{
    constexpr frame_info frame_info{1, 1, 8, 1};
    constexpr coding_parameters parameters{};

    array<byte, 4> buffer{};

    scan_decoder_tester scan_decoder(frame_info, parameters, buffer.data(), buffer.size());
    EXPECT_EQ(-1, scan_decoder.peek_0_bits_forward());
}

TEST(scan_decoder_test, initialize_with_empty_buffer_throws_invalid_data)
{
    constexpr frame_info frame_info{1, 1, 8, 1};
    constexpr coding_parameters parameters{};

    array<byte, 1> buffer{};

    assert_expect_exception(jpegls_errc::invalid_data, [&frame_info, &parameters, &buffer] {
        [[maybe_unused]] const scan_decoder_tester scan_decoder(frame_info, parameters, buffer.data(), 0);
    });
}

} // namespace charls::test
