// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "../src/decoder_strategy.h"

#include "encoder_strategy_tester.h"

#include <array>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;
using std::unique_ptr;

namespace charls { namespace test {

namespace {

class decoder_strategy_tester final : public charls::decoder_strategy
{
public:
    decoder_strategy_tester(const charls::frame_info& frame_info, const charls::coding_parameters& parameters,
                            uint8_t* const destination, const size_t count) :
        // NOLINT
        decoder_strategy(frame_info, parameters)
    {
        initialize({destination, count});
    }

    void set_presets(const charls::jpegls_pc_parameters& /*preset_coding_parameters*/,
                     uint32_t /*restart_interval*/) noexcept(false) override
    {
    }

    unique_ptr<charls::process_line> create_process_line(charls::byte_span /*rawStreamInfo*/,
                                                         size_t /*stride*/) noexcept(false) override
    {
        return nullptr;
    }

    size_t decode_scan(unique_ptr<charls::process_line> /*process_line*/, const JlsRect& /*size*/,
                       charls::const_byte_span /*encoded_source*/) noexcept(false) override
    {
        return {};
    }

    int32_t read(const int32_t length)
    {
        return read_long_value(length);
    }
};

} // namespace


TEST_CLASS(decoder_strategy_test)
{
public:
    TEST_METHOD(decode_encoded_ff_pattern) // NOLINT
    {
        struct data_t final
        {
            int32_t value;
            int bits;
        };

        constexpr array<data_t, 5> in_data{{{0x00, 24}, {0xFF, 8}, {0xFFFF, 16}, {0xFFFF, 16}, {0x12345678, 31}}};

        array<uint8_t, 100> enc_buf{};
        constexpr frame_info frame_info{};
        constexpr coding_parameters parameters{};

        encoder_strategy_tester encoder(frame_info, parameters);

        encoder.initialize_forward({enc_buf.data(), enc_buf.size()});

        for (const auto& data : in_data)
        {
            encoder.append_to_bit_stream_forward(data.value, data.bits);
        }

        encoder.end_scan_forward();
        // Note: Correct encoding is tested in encoder_strategy_test::append_to_bit_stream_ff_pattern.

        const auto length{encoder.get_length_forward()};
        decoder_strategy_tester decoder(frame_info, parameters, enc_buf.data(), length);
        for (const auto& data : in_data)
        {
            const auto actual{decoder.read(data.bits)};
            Assert::AreEqual(data.value, actual);
        }
    }

    TEST_METHOD(peek_byte) // NOLINT
    {
        constexpr frame_info frame_info{};
        constexpr coding_parameters parameters{};

        array<uint8_t, 4> buffer{7, 100, 23, 99};

        decoder_strategy_tester decoder_strategy(frame_info, parameters, buffer.data(), buffer.size());

        Assert::AreEqual(7, decoder_strategy.peek_byte());
    }

    TEST_METHOD(read_bit) // NOLINT
    {
        constexpr frame_info frame_info{};
        constexpr coding_parameters parameters{};

        array<uint8_t, 4> buffer{0xAA, 100, 23, 99};

        decoder_strategy_tester decoder_strategy(frame_info, parameters, buffer.data(), buffer.size());

        Assert::IsTrue(decoder_strategy.read_bit());
        Assert::IsFalse(decoder_strategy.read_bit());
        Assert::IsTrue(decoder_strategy.read_bit());
        Assert::IsFalse(decoder_strategy.read_bit());
        Assert::IsTrue(decoder_strategy.read_bit());
        Assert::IsFalse(decoder_strategy.read_bit());
        Assert::IsTrue(decoder_strategy.read_bit());
        Assert::IsFalse(decoder_strategy.read_bit());
    }

    TEST_METHOD(peek_0_bits) // NOLINT
    {
        constexpr frame_info frame_info{};
        constexpr coding_parameters parameters{};

        {
            array<uint8_t, 4> buffer{0xF, 100, 23, 99};

            decoder_strategy_tester decoder_strategy(frame_info, parameters, buffer.data(), buffer.size());
            Assert::AreEqual(4, decoder_strategy.peek_0_bits());
        }

        {
            array<uint8_t, 4> buffer{0, 1, 0, 0};

            decoder_strategy_tester decoder_strategy(frame_info, parameters, buffer.data(), buffer.size());
            Assert::AreEqual(15, decoder_strategy.peek_0_bits());
        }

        {
            array<uint8_t, 4> buffer{0, 0, 0, 0};

            decoder_strategy_tester decoder_strategy(frame_info, parameters, buffer.data(), buffer.size());
            Assert::AreEqual(-1, decoder_strategy.peek_0_bits());
        }
    }
};

}} // namespace charls::test
