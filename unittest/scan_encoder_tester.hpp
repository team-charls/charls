// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "../src/scan_encoder.hpp"

namespace charls::test {

class scan_encoder_tester final : scan_encoder
{
public:
    explicit scan_encoder_tester(const charls::frame_info& frame_info, const coding_parameters& parameters) noexcept :
        scan_encoder(frame_info, {}, parameters, nullptr)
    {
    }

    size_t encode_scan(const std::byte* /*source*/, size_t /*stride*/, span<std::byte>) noexcept(false) override
    {
        return 0;
    }

    void initialize_forward(const span<std::byte> destination) noexcept
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

    [[nodiscard]]
    size_t get_length_forward() const noexcept
    {
        return get_length();
    }

    void end_scan_forward()
    {
        end_scan();
    }
};

} // namespace charls::test
