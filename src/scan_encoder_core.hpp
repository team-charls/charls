// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "coding_parameters.hpp"
#include "jpegls_algorithm.hpp"
#include "scan_encoder.hpp"

namespace charls {

/// <summary>
/// Intermediate template class parameterized on sample-level traits only.
/// Contains the encoder functions that depend only on sample-level properties, not on pixel type.
/// </summary>
template<typename SampleTraits>
class scan_encoder_core : public scan_encoder
{
public:
    using sample_type = typename SampleTraits::sample_type;

protected:
    scan_encoder_core(const charls::frame_info& frame_info, const jpegls_pc_parameters& pc_parameters,
                      const coding_parameters& parameters, const copy_to_line_buffer_fn copy_to_line_buffer,
                      const SampleTraits& sample_traits) :
        scan_encoder{frame_info, pc_parameters, parameters, copy_to_line_buffer}, sample_traits_{sample_traits}
    {
        quantization_ = initialize_quantization_lut(sample_traits_, t1_, t2_, t3_, quantization_lut_);
        initialize_parameters(sample_traits_.range);
    }

    [[nodiscard]]
    FORCE_INLINE int32_t quantize_gradient(const int32_t di) const noexcept
    {
        ASSERT(this->quantize_gradient_org(di, sample_traits_.near_lossless) == *(quantization_ + di));
        return *(quantization_ + di);
    }

    [[nodiscard]]
    sample_type encode_regular(const int32_t qs, const int32_t x, const int32_t predicted)
    {
        const int32_t sign{bit_wise_sign(qs)};
        regular_mode_context& context{regular_mode_contexts_[apply_sign_for_index(qs, sign)]};
        const int32_t k{context.compute_golomb_coding_parameter_for_encoder()};
        const int32_t predicted_value{sample_traits_.correct_prediction(predicted + apply_sign(context.c(), sign))};
        const int32_t error_value{sample_traits_.compute_error_value(apply_sign(x - predicted_value, sign))};

        encode_mapped_value(k, map_error_value(context.get_error_correction(k | sample_traits_.near_lossless) ^ error_value),
                            sample_traits_.limit);
        context.update_variables_and_bias(error_value, sample_traits_.near_lossless, reset_threshold_);
        ASSERT(sample_traits_.is_near(
            sample_traits_.compute_reconstructed_sample(predicted_value, apply_sign(error_value, sign)), x));
        return static_cast<sample_type>(
            sample_traits_.compute_reconstructed_sample(predicted_value, apply_sign(error_value, sign)));
    }

    void encode_regular_lossless(const int32_t qs, const int32_t x, const int32_t predicted)
    {
        const int32_t sign{bit_wise_sign(qs)};
        regular_mode_context& context{regular_mode_contexts_[apply_sign_for_index(qs, sign)]};
        const int32_t k{context.compute_golomb_coding_parameter_for_encoder()};
        const int32_t predicted_value{sample_traits_.correct_prediction(predicted + apply_sign(context.c(), sign))};
        const int32_t error_value{sample_traits_.compute_error_value(apply_sign(x - predicted_value, sign))};

        encode_mapped_value(k, map_error_value(context.get_error_correction(k) ^ error_value), sample_traits_.limit);
        context.update_variables_and_bias(error_value, 0, reset_threshold_);
    }

    void encode_mapped_value(const int32_t k, const int32_t mapped_error, const int32_t limit)
    {
        if (int32_t high_bits{mapped_error >> k}; high_bits < limit - sample_traits_.quantized_bits_per_sample - 1)
        {
            if (high_bits + 1 > 31)
            {
                append_to_bit_stream(0, high_bits / 2);
                high_bits = high_bits - high_bits / 2;
            }
            if (const int32_t total_bits{high_bits + 1 + k}; total_bits < 32)
            {
                // Merge unary prefix (high_bits zeros + 1) and k-bit remainder into a single call.
                append_to_bit_stream((1U << k) | static_cast<uint32_t>(mapped_error & ((1 << k) - 1)), total_bits);
            }
            else
            {
                append_to_bit_stream(1, high_bits + 1);
                append_to_bit_stream(static_cast<uint32_t>(mapped_error & ((1 << k) - 1)), k);
            }
            return;
        }

        if (limit - sample_traits_.quantized_bits_per_sample > 31)
        {
            append_to_bit_stream(0, 31);
            append_to_bit_stream(1, limit - sample_traits_.quantized_bits_per_sample - 31);
        }
        else
        {
            append_to_bit_stream(1, limit - sample_traits_.quantized_bits_per_sample);
        }
        append_to_bit_stream(
            static_cast<uint32_t>((mapped_error - 1) & ((1 << sample_traits_.quantized_bits_per_sample) - 1)),
            sample_traits_.quantized_bits_per_sample);
    }

    void encode_run_interruption_error(run_mode_context& context, const int32_t error_value)
    {
        const int32_t k{context.compute_golomb_coding_parameter()};
        const bool map{context.compute_map(error_value, k)};
        const int32_t e_mapped_error_value{2 * std::abs(error_value) - context.run_interruption_type() -
                                           static_cast<int32_t>(map)};

        ASSERT(error_value == context.compute_error_value(e_mapped_error_value + context.run_interruption_type(), k));
        encode_mapped_value(k, e_mapped_error_value, sample_traits_.limit - J[run_index_] - 1);
        context.update_variables(error_value, e_mapped_error_value, reset_threshold_);
    }

    [[nodiscard]]
    sample_type encode_run_interruption_pixel(const int32_t x, const int32_t ra, const int32_t rb)
    {
        if (std::abs(ra - rb) <= sample_traits_.near_lossless)
        {
            const int32_t error_value{sample_traits_.compute_error_value(x - ra)};
            encode_run_interruption_error(run_mode_contexts_[1], error_value);
            return static_cast<sample_type>(sample_traits_.compute_reconstructed_sample(ra, error_value));
        }

        const int32_t error_value{sample_traits_.compute_error_value((x - rb) * sign(rb - ra))};
        encode_run_interruption_error(run_mode_contexts_[0], error_value);
        return static_cast<sample_type>(sample_traits_.compute_reconstructed_sample(rb, error_value * sign(rb - ra)));
    }

    [[nodiscard]]
    sample_type encode_run_interruption_component(const int32_t x, const int32_t ra, const int32_t rb)
    {
        const int32_t error_value{sample_traits_.compute_error_value(sign(rb - ra) * (x - rb))};
        encode_run_interruption_error(run_mode_contexts_[0], error_value);
        return sample_traits_.compute_reconstructed_sample(rb, error_value * sign(rb - ra));
    }

    SampleTraits sample_traits_;
};

} // namespace charls
