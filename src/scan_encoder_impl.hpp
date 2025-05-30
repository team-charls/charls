// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "coding_parameters.hpp"
#include "jpegls_algorithm.hpp"
#include "scan_encoder.hpp"

namespace charls {

template<typename Traits>
class scan_encoder_impl final : public scan_encoder
{
public:
    using sample_type = typename Traits::sample_type;
    using pixel_type = typename Traits::pixel_type;

    scan_encoder_impl(const charls::frame_info& frame_info, const jpegls_pc_parameters& pc_parameters,
                      const coding_parameters& parameters, const Traits& traits) :
        scan_encoder{
            frame_info, pc_parameters, parameters,
            copy_to_line_buffer<sample_type>::get_copy_function(parameters.interleave_mode, frame_info.component_count,
                                                                frame_info.bits_per_sample, parameters.transformation)},
        traits_{traits}
    {
        ASSERT(traits_.is_valid());

        quantization_ = initialize_quantization_lut(traits_, t1_, t2_, t3_, quantization_lut_);
        initialize_parameters(traits_.range);
    }

    size_t encode_scan(const std::byte* source, const size_t stride, const span<std::byte> destination) override
    {
        initialize(destination);
        encode_lines(source, stride);
        end_scan();

        return get_length();
    }

private:
    [[nodiscard]]
    FORCE_INLINE int32_t quantize_gradient(const int32_t di) const noexcept
    {
        ASSERT(quantize_gradient_org(di, traits_.near_lossless) == *(quantization_ + di));
        return *(quantization_ + di);
    }

    // In ILV_SAMPLE mode, multiple components are handled in do_line
    // In ILV_LINE mode, a call to do_line is made for every component
    // In ILV_NONE mode, do_scan is called for each component
    void encode_lines(const std::byte* source, const size_t stride)
    {
        const uint32_t pixel_stride{width_ + 2U};
        const size_t component_count{
            parameters().interleave_mode == interleave_mode::line ? static_cast<size_t>(frame_info().component_count) : 1U};

        std::array<uint32_t, maximum_component_count_in_scan> run_index{};
        std::vector<pixel_type> line_buffer(component_count * pixel_stride * 2);

        for (uint32_t line{}; line < frame_info().height; ++line)
        {
            previous_line_ = line_buffer.data();
            current_line_ = line_buffer.data() + static_cast<size_t>(component_count) * pixel_stride;
            if ((line & 1) == 1)
            {
                std::swap(previous_line_, current_line_);
            }

            copy_source_to_line_buffer(source, current_line_ + 1, width_);
            source = source + stride;

            for (size_t component{}; component < component_count; ++component)
            {
                run_index_ = run_index[component];

                initialize_edge_pixels(previous_line_, current_line_, width_);

                if constexpr (std::is_same_v<pixel_type, sample_type>)
                {
                    encode_sample_line();
                }
                else if constexpr (std::is_same_v<pixel_type, pair<sample_type>>)
                {
                    encode_pair_line();
                }
                else if constexpr (std::is_same_v<pixel_type, triplet<sample_type>>)
                {
                    encode_triplet_line();
                }
                else
                {
                    static_assert(std::is_same_v<pixel_type, quad<sample_type>>);
                    encode_quad_line();
                }

                run_index[component] = run_index_;
                previous_line_ += pixel_stride;
                current_line_ += pixel_stride;
            }
        }
    }

    /// <summary>Encodes a scan line of samples</summary>
    FORCE_INLINE void encode_sample_line()
    {
        size_t index{1};
        int32_t rb{previous_line_[index - 1]};
        int32_t rd{previous_line_[index]};

        while (index <= width_)
        {
            const int32_t ra{current_line_[index - 1]};
            const int32_t rc{rb};
            rb = rd;
            rd = previous_line_[index + 1];

            if (const int32_t qs{
                    compute_context_id(quantize_gradient(rd - rb), quantize_gradient(rb - rc), quantize_gradient(rc - ra))};
                qs != 0)
            {
                current_line_[index] = encode_regular(qs, current_line_[index], compute_predicted_value(ra, rb, rc));
                ++index;
            }
            else
            {
                index += encode_run_mode(index);
                rb = previous_line_[index - 1];
                rd = previous_line_[index];
            }
        }
    }

    /// <summary>Encodes a scan line of pairs in ILV_SAMPLE mode</summary>
    void encode_pair_line()
    {
        size_t index{1};
        while (index <= width_)
        {
            const pair<sample_type> ra{current_line_[index - 1]};
            const pair<sample_type> rc{previous_line_[index - 1]};
            const pair<sample_type> rb{previous_line_[index]};
            const pair<sample_type> rd{previous_line_[index + 1]};

            const int32_t qs1{compute_context_id(quantize_gradient(rd.v1 - rb.v1), quantize_gradient(rb.v1 - rc.v1),
                                                 quantize_gradient(rc.v1 - ra.v1))};
            const int32_t qs2{compute_context_id(quantize_gradient(rd.v2 - rb.v2), quantize_gradient(rb.v2 - rc.v2),
                                                 quantize_gradient(rc.v2 - ra.v2))};

            if (qs1 == 0 && qs2 == 0)
            {
                index += encode_run_mode(index);
            }
            else
            {
                pair<sample_type> rx;
                rx.v1 = encode_regular(qs1, current_line_[index].v1, compute_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = encode_regular(qs2, current_line_[index].v2, compute_predicted_value(ra.v2, rb.v2, rc.v2));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    /// <summary>Encodes a scan line of triplets in ILV_SAMPLE mode</summary>
    void encode_triplet_line()
    {
        size_t index{1};
        while (index <= width_)
        {
            const triplet<sample_type> ra{current_line_[index - 1]};
            const triplet<sample_type> rc{previous_line_[index - 1]};
            const triplet<sample_type> rb{previous_line_[index]};
            const triplet<sample_type> rd{previous_line_[index + 1]};

            const int32_t qs1{compute_context_id(quantize_gradient(rd.v1 - rb.v1), quantize_gradient(rb.v1 - rc.v1),
                                                 quantize_gradient(rc.v1 - ra.v1))};
            const int32_t qs2{compute_context_id(quantize_gradient(rd.v2 - rb.v2), quantize_gradient(rb.v2 - rc.v2),
                                                 quantize_gradient(rc.v2 - ra.v2))};

            if (const int32_t qs3{compute_context_id(quantize_gradient(rd.v3 - rb.v3), quantize_gradient(rb.v3 - rc.v3),
                                                     quantize_gradient(rc.v3 - ra.v3))};
                qs1 == 0 && qs2 == 0 && qs3 == 0)
            {
                index += encode_run_mode(index);
            }
            else
            {
                triplet<sample_type> rx;
                rx.v1 = encode_regular(qs1, current_line_[index].v1, compute_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = encode_regular(qs2, current_line_[index].v2, compute_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = encode_regular(qs3, current_line_[index].v3, compute_predicted_value(ra.v3, rb.v3, rc.v3));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    /// <summary>Encodes a scan line of quads in ILV_SAMPLE mode</summary>
    void encode_quad_line()
    {
        size_t index{1};
        while (index <= width_)
        {
            const quad<sample_type> ra{current_line_[index - 1]};
            const quad<sample_type> rc{previous_line_[index - 1]};
            const quad<sample_type> rb{previous_line_[index]};
            const quad<sample_type> rd{previous_line_[index + 1]};

            const int32_t qs1{compute_context_id(quantize_gradient(rd.v1 - rb.v1), quantize_gradient(rb.v1 - rc.v1),
                                                 quantize_gradient(rc.v1 - ra.v1))};
            const int32_t qs2{compute_context_id(quantize_gradient(rd.v2 - rb.v2), quantize_gradient(rb.v2 - rc.v2),
                                                 quantize_gradient(rc.v2 - ra.v2))};
            const int32_t qs3{compute_context_id(quantize_gradient(rd.v3 - rb.v3), quantize_gradient(rb.v3 - rc.v3),
                                                 quantize_gradient(rc.v3 - ra.v3))};

            if (const int32_t qs4{compute_context_id(quantize_gradient(rd.v4 - rb.v4), quantize_gradient(rb.v4 - rc.v4),
                                                     quantize_gradient(rc.v4 - ra.v4))};
                qs1 == 0 && qs2 == 0 && qs3 == 0 && qs4 == 0)
            {
                index += encode_run_mode(index);
            }
            else
            {
                quad<sample_type> rx;
                rx.v1 = encode_regular(qs1, current_line_[index].v1, compute_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = encode_regular(qs2, current_line_[index].v2, compute_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = encode_regular(qs3, current_line_[index].v3, compute_predicted_value(ra.v3, rb.v3, rc.v3));
                rx.v4 = encode_regular(qs4, current_line_[index].v4, compute_predicted_value(ra.v4, rb.v4, rc.v4));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    [[nodiscard]]
    size_t encode_run_mode(const size_t start_index)
    {
        const size_t count_type_remain = width_ - (start_index - 1);
        pixel_type* type_cur_x{current_line_ + start_index};
        const pixel_type* type_prev_x{previous_line_ + start_index};

        const pixel_type ra{type_cur_x[-1]};

        size_t run_length{};
        while (traits_.is_near(type_cur_x[run_length], ra))
        {
            type_cur_x[run_length] = ra;
            ++run_length;

            if (run_length == count_type_remain)
                break;
        }

        encode_run_pixels(run_length, run_length == count_type_remain);

        if (run_length == count_type_remain)
            return run_length;

        type_cur_x[run_length] = encode_run_interruption_pixel(type_cur_x[run_length], ra, type_prev_x[run_length]);
        decrement_run_index();
        return run_length + 1;
    }

    [[nodiscard]]
    FORCE_INLINE sample_type encode_regular(const int32_t qs, const int32_t x, const int32_t predicted)
    {
        const int32_t sign{bit_wise_sign(qs)};
        regular_mode_context& context{regular_mode_contexts_[apply_sign_for_index(qs, sign)]};
        const int32_t k{context.compute_golomb_coding_parameter()};
        const int32_t predicted_value{traits_.correct_prediction(predicted + apply_sign(context.c(), sign))};
        const int32_t error_value{traits_.compute_error_value(apply_sign(x - predicted_value, sign))};

        encode_mapped_value(k, map_error_value(context.get_error_correction(k | traits_.near_lossless) ^ error_value),
                            traits_.limit);
        context.update_variables_and_bias(error_value, traits_.near_lossless, reset_threshold_);
        ASSERT(traits_.is_near(traits_.compute_reconstructed_sample(predicted_value, apply_sign(error_value, sign)), x));
        return static_cast<sample_type>(
            traits_.compute_reconstructed_sample(predicted_value, apply_sign(error_value, sign)));
    }

    FORCE_INLINE void encode_mapped_value(const int32_t k, const int32_t mapped_error, const int32_t limit)
    {
        if (int32_t high_bits{mapped_error >> k}; high_bits < limit - traits_.quantized_bits_per_sample - 1)
        {
            if (high_bits + 1 > 31)
            {
                append_to_bit_stream(0, high_bits / 2);
                high_bits = high_bits - high_bits / 2;
            }
            append_to_bit_stream(1, high_bits + 1);
            append_to_bit_stream(static_cast<uint32_t>(mapped_error & ((1 << k) - 1)), k);
            return;
        }

        if (limit - traits_.quantized_bits_per_sample > 31)
        {
            append_to_bit_stream(0, 31);
            append_to_bit_stream(1, limit - traits_.quantized_bits_per_sample - 31);
        }
        else
        {
            append_to_bit_stream(1, limit - traits_.quantized_bits_per_sample);
        }
        append_to_bit_stream(static_cast<uint32_t>((mapped_error - 1) & ((1 << traits_.quantized_bits_per_sample) - 1)),
                             traits_.quantized_bits_per_sample);
    }

    void encode_run_interruption_error(run_mode_context& context, const int32_t error_value)
    {
        const int32_t k{context.compute_golomb_coding_parameter()};
        const bool map{context.compute_map(error_value, k)};
        const int32_t e_mapped_error_value{2 * std::abs(error_value) - context.run_interruption_type() -
                                           static_cast<int32_t>(map)};

        ASSERT(error_value == context.compute_error_value(e_mapped_error_value + context.run_interruption_type(), k));
        encode_mapped_value(k, e_mapped_error_value, traits_.limit - J[run_index_] - 1);
        context.update_variables(error_value, e_mapped_error_value, reset_threshold_);
    }

    [[nodiscard]]
    sample_type encode_run_interruption_pixel(const int32_t x, const int32_t ra, const int32_t rb)
    {
        if (std::abs(ra - rb) <= traits_.near_lossless)
        {
            const int32_t error_value{traits_.compute_error_value(x - ra)};
            encode_run_interruption_error(run_mode_contexts_[1], error_value);
            return static_cast<sample_type>(traits_.compute_reconstructed_sample(ra, error_value));
        }

        const int32_t error_value{traits_.compute_error_value((x - rb) * sign(rb - ra))};
        encode_run_interruption_error(run_mode_contexts_[0], error_value);
        return static_cast<sample_type>(traits_.compute_reconstructed_sample(rb, error_value * sign(rb - ra)));
    }

    [[nodiscard]]
    pair<sample_type> encode_run_interruption_pixel(const pair<sample_type> x, const pair<sample_type> ra,
                                                    const pair<sample_type> rb)
    {
        const int32_t error_value1{traits_.compute_error_value(sign(rb.v1 - ra.v1) * (x.v1 - rb.v1))};
        encode_run_interruption_error(run_mode_contexts_[0], error_value1);

        const int32_t error_value2{traits_.compute_error_value(sign(rb.v2 - ra.v2) * (x.v2 - rb.v2))};
        encode_run_interruption_error(run_mode_contexts_[0], error_value2);

        return {traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2))};
    }

    [[nodiscard]]
    triplet<sample_type> encode_run_interruption_pixel(const triplet<sample_type> x, const triplet<sample_type> ra,
                                                       const triplet<sample_type> rb)
    {
        const int32_t error_value1{traits_.compute_error_value(sign(rb.v1 - ra.v1) * (x.v1 - rb.v1))};
        encode_run_interruption_error(run_mode_contexts_[0], error_value1);

        const int32_t error_value2{traits_.compute_error_value(sign(rb.v2 - ra.v2) * (x.v2 - rb.v2))};
        encode_run_interruption_error(run_mode_contexts_[0], error_value2);

        const int32_t error_value3{traits_.compute_error_value(sign(rb.v3 - ra.v3) * (x.v3 - rb.v3))};
        encode_run_interruption_error(run_mode_contexts_[0], error_value3);

        return {traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3))};
    }

    [[nodiscard]]
    quad<sample_type> encode_run_interruption_pixel(const quad<sample_type> x, const quad<sample_type> ra,
                                                    const quad<sample_type> rb)
    {
        const int32_t error_value1{traits_.compute_error_value(sign(rb.v1 - ra.v1) * (x.v1 - rb.v1))};
        encode_run_interruption_error(run_mode_contexts_[0], error_value1);

        const int32_t error_value2{traits_.compute_error_value(sign(rb.v2 - ra.v2) * (x.v2 - rb.v2))};
        encode_run_interruption_error(run_mode_contexts_[0], error_value2);

        const int32_t error_value3{traits_.compute_error_value(sign(rb.v3 - ra.v3) * (x.v3 - rb.v3))};
        encode_run_interruption_error(run_mode_contexts_[0], error_value3);

        const int32_t error_value4{traits_.compute_error_value(sign(rb.v4 - ra.v4) * (x.v4 - rb.v4))};
        encode_run_interruption_error(run_mode_contexts_[0], error_value4);

        return {traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3)),
                traits_.compute_reconstructed_sample(rb.v4, error_value4 * sign(rb.v4 - ra.v4))};
    }

    Traits traits_;
    pixel_type* previous_line_{};
    pixel_type* current_line_{};
};

} // namespace charls
