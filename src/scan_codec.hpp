// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "coding_parameters.hpp"
#include "jpegls_algorithm.hpp"
#include "quantization_lut.hpp"
#include "regular_mode_context.hpp"
#include "run_mode_context.hpp"

#include <array>

namespace charls {

// Used to determine how large runs should be encoded at a time. Defined by the JPEG-LS standard, A.2.1., Initialization
// step 3.
inline constexpr std::array<int, 32> J{
    {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 11, 12, 13, 14, 15}};


template<typename Traits>
bool precomputed_quantization_lut_available(const Traits& traits, const int32_t threshold1, const int32_t threshold2,
                                            const int32_t threshold3) noexcept
{
    if (const jpegls_pc_parameters presets{compute_default(traits.maximum_sample_value, traits.near_lossless)};
        presets.threshold1 != threshold1 || presets.threshold2 != threshold2 || presets.threshold3 != threshold3)
        return false;

    if constexpr (Traits::always_lossless_and_default_parameters)
        return true;
    else
        return traits.near_lossless == 0 && traits.maximum_sample_value == (1 << traits.bits_per_sample) - 1;
}


template<typename Traits>
const int8_t* initialize_quantization_lut(const Traits& traits, const int32_t threshold1, const int32_t threshold2,
                                          const int32_t threshold3, std::vector<int8_t>& quantization_lut)
{
    // For lossless mode with default parameters, we have precomputed the lookup table for bit counts 8, 10, 12 and 16.
    if (precomputed_quantization_lut_available(traits, threshold1, threshold2, threshold3))
    {
        if constexpr (Traits::fixed_bits_per_pixel)
        {
            if constexpr (Traits::bits_per_sample == 8)
                return &quantization_lut_lossless_8[quantization_lut_lossless_8.size() / 2];
            else
            {
                if constexpr (Traits::bits_per_sample == 12)
                    return &quantization_lut_lossless_12[quantization_lut_lossless_12.size() / 2];
                else
                {
                    static_assert(Traits::bits_per_sample == 16);
                    return &quantization_lut_lossless_16[quantization_lut_lossless_16.size() / 2];
                }
            }
        }
        else
        {
            switch (traits.bits_per_sample)
            {
            case 8:
                return &quantization_lut_lossless_8[quantization_lut_lossless_8.size() / 2];
            case 10:
                return &quantization_lut_lossless_10[quantization_lut_lossless_10.size() / 2];
            case 12:
                return &quantization_lut_lossless_12[quantization_lut_lossless_12.size() / 2];
            case 16:
                return &quantization_lut_lossless_16[quantization_lut_lossless_16.size() / 2];
            default:
                break;
            }
        }
    }

    // Initialize the quantization lookup table dynamic.
    quantization_lut.resize(size_t{2} * traits.quantization_range);
    for (size_t i{}; i < quantization_lut.size(); ++i)
    {
        quantization_lut[i] =
            quantize_gradient_org(-static_cast<int32_t>(traits.quantization_range) + static_cast<int32_t>(i), threshold1,
                                  threshold2, threshold3, traits.near_lossless);
    }

    return &quantization_lut[traits.quantization_range];
}


[[nodiscard]]
constexpr size_t pixel_count_to_pixel_stride(const size_t pixel_count) noexcept
{
    // The line buffer is allocated with 2 extra pixels for the edges.
    return pixel_count + 2;
}


/// <summary>
/// Base class for scan_encoder and scan_decoder
/// Contains the variables and methods that are identical for the encoding/decoding process and can be shared.
/// </summary>
class scan_codec
{
public:
    scan_codec(const scan_codec&) = delete;
    scan_codec(scan_codec&&) = delete;
    scan_codec& operator=(const scan_codec&) = delete;
    scan_codec& operator=(scan_codec&&) = delete;

protected:
    /// <remarks>
    /// Copy frame_info and parameters to prevent 1 indirection during encoding/decoding.
    /// </remarks>
    scan_codec(const frame_info& frame_info, const jpegls_pc_parameters& pc_parameters,
               const coding_parameters& parameters) noexcept :
        frame_info_{frame_info},
        parameters_{parameters},
        t1_{pc_parameters.threshold1},
        t2_{pc_parameters.threshold2},
        t3_{pc_parameters.threshold3},
        width_{frame_info.width},
        reset_threshold_{static_cast<uint8_t>(pc_parameters.reset_value)}
    {
        ASSERT((parameters.interleave_mode == interleave_mode::none && this->frame_info().component_count == 1) ||
               parameters.interleave_mode != interleave_mode::none);
    }

    ~scan_codec() = default;

    [[nodiscard]]
    int8_t quantize_gradient_org(const int32_t di, const int32_t near_lossless) const noexcept
    {
        return charls::quantize_gradient_org(di, t1_, t2_, t3_, near_lossless);
    }

    [[nodiscard]]
    const coding_parameters& parameters() const noexcept
    {
        return parameters_;
    }

    [[nodiscard]]
    const charls::frame_info& frame_info() const noexcept
    {
        return frame_info_;
    }

    void initialize_parameters(const int32_t range) noexcept
    {
        const regular_mode_context context_initial_value(range);
        for (auto& context : regular_mode_contexts_)
        {
            context = context_initial_value;
        }

        run_mode_contexts_[0] = run_mode_context(0, range);
        run_mode_contexts_[1] = run_mode_context(1, range);
        run_index_ = 0;
    }

    void increment_run_index() noexcept
    {
        if (run_index_ < 31U)
        {
            ++run_index_;
        }
    }

    void decrement_run_index() noexcept
    {
        if (run_index_ > 0)
        {
            --run_index_;
        }
    }

    template<typename PixelType>
    static void initialize_edge_pixels(PixelType* previous_line, PixelType* current_line, const uint32_t width) noexcept
    {
        // Initialize edge pixels used for prediction
        previous_line[width + 1] = previous_line[width];
        current_line[0] = previous_line[1];
    }

    charls::frame_info frame_info_;
    coding_parameters parameters_;
    int32_t t1_{};
    int32_t t2_{};
    int32_t t3_{};
    uint32_t run_index_{};
    std::array<regular_mode_context, 365> regular_mode_contexts_;
    std::array<run_mode_context, 2> run_mode_contexts_;
    uint32_t width_;

    // ISO 14495-1 RESET symbol: threshold value at which A, B, and N are halved.
    int32_t reset_threshold_{};

    // Quantization lookup table
    const int8_t* quantization_{};
    std::vector<int8_t> quantization_lut_;
};

} // namespace charls
