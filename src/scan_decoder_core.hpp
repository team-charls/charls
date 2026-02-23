// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "golomb_lut.hpp"
#include "scan_decoder.hpp"

namespace charls {

/// <summary>
/// Intermediate template class parameterized on sample-level traits only.
/// Contains the heavy decoder functions (decode_regular, decode_run_interruption_error, etc.)
/// that depend only on sample-level properties, not on pixel type.
/// This reduces template instantiations from 17 to 7 per decoder.
/// </summary>
template<typename SampleTraits>
class scan_decoder_core : public scan_decoder
{
public:
    using sample_type = typename SampleTraits::sample_type;

protected:
    scan_decoder_core(const charls::frame_info& frame_info, const jpegls_pc_parameters& pc_parameters,
                      const coding_parameters& parameters, const SampleTraits& sample_traits) :
        scan_decoder{frame_info, pc_parameters, parameters}, sample_traits_{sample_traits}
    {
        quantization_ = initialize_quantization_lut(sample_traits_, t1_, t2_, t3_, quantization_lut_);
        initialize_parameters(sample_traits_.range);
    }

    [[nodiscard]]
    sample_type decode_regular(const int32_t qs, const int32_t predicted)
    {
        const int32_t sign{bit_wise_sign(qs)};
        regular_mode_context& context{regular_mode_contexts_[apply_sign_for_index(qs, sign)]};
        const int32_t corrected_prediction{sample_traits_.correct_prediction(predicted + apply_sign(context.c(), sign))};
        const int32_t k{context.compute_golomb_coding_parameter()};

        int32_t error_value;
        if (const golomb_code_match code = golomb_lut[static_cast<size_t>(k)].get(peek_byte()); code.bit_count != 0)
        {
            // There is a pre-computed match.
            skip_bits(code.bit_count);
            error_value = code.error_value;
            ASSERT(std::abs(error_value) < 65535);
        }
        else
        {
            error_value =
                unmap_error_value(decode_mapped_error_value(k, sample_traits_.limit, sample_traits_.quantized_bits_per_sample));
            if (UNLIKELY(std::abs(error_value) > 65535))
                impl::throw_jpegls_error(jpegls_errc::invalid_data);
        }

        if (k == 0)
        {
            error_value = error_value ^ context.get_error_correction(sample_traits_.near_lossless);
        }

        context.update_variables_and_bias(error_value, sample_traits_.near_lossless, reset_threshold_);
        error_value = apply_sign(error_value, sign);
        return sample_traits_.compute_reconstructed_sample(corrected_prediction, error_value);
    }

    [[nodiscard]]
    int32_t decode_run_interruption_error(run_mode_context& context)
    {
        const int32_t k{context.compute_golomb_coding_parameter_checked()};
        const int32_t e_mapped_error_value{
            decode_mapped_error_value(k, sample_traits_.limit - J[run_index_] - 1, sample_traits_.quantized_bits_per_sample)};
        const int32_t error_value{context.compute_error_value(e_mapped_error_value + context.run_interruption_type(), k)};
        context.update_variables(error_value, e_mapped_error_value, reset_threshold_);
        return error_value;
    }

    [[nodiscard]]
    sample_type decode_run_interruption_pixel(const int32_t ra, const int32_t rb)
    {
        if (std::abs(ra - rb) <= sample_traits_.near_lossless)
        {
            const int32_t error_value{decode_run_interruption_error(run_mode_contexts_[1])};
            return static_cast<sample_type>(sample_traits_.compute_reconstructed_sample(ra, error_value));
        }

        const int32_t error_value{decode_run_interruption_error(run_mode_contexts_[0])};
        return static_cast<sample_type>(sample_traits_.compute_reconstructed_sample(rb, error_value * sign(rb - ra)));
    }

    [[nodiscard]]
    sample_type decode_run_interruption_component(const int32_t ra, const int32_t rb)
    {
        const int32_t error_value{decode_run_interruption_error(run_mode_contexts_[0])};
        return sample_traits_.compute_reconstructed_sample(rb, error_value * sign(rb - ra));
    }

    SampleTraits sample_traits_;
};

} // namespace charls
