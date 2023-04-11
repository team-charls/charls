// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "../src/encoder_strategy.h"

namespace charls::test {

class encoder_strategy_tester final : encoder_strategy
{
public:
    explicit encoder_strategy_tester(const frame_info& frame_info, const coding_parameters& parameters) noexcept :
        encoder_strategy(frame_info, parameters)
    {
    }

    void set_presets(const jpegls_pc_parameters&) noexcept(false) override
    {
    }

    size_t encode_scan(const_byte_span /*source*/, size_t /*stride*/, byte_span) noexcept(false) override
    {
        return 0;
    }

    void initialize_forward(const byte_span destination) noexcept
    {
        initialize(destination);
    }

    void append_to_bit_stream_forward(const uint32_t bits, const int32_t bit_count)
    {
        append_to_bit_stream(bits, bit_count);
    }

    void flush_forward()
    {
        flush();
    }

    [[nodiscard]] size_t get_length_forward() const noexcept
    {
        return get_length();
    }

    void end_scan_forward()
    {
        end_scan();
    }
};

} // namespace charls::test
