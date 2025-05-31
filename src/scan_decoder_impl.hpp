// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "color_transform.hpp"
#include "golomb_lut.hpp"
#include "scan_decoder.hpp"

namespace charls {

template<typename Traits>
class scan_decoder_impl final : public scan_decoder
{
public:
    using pixel_type = typename Traits::pixel_type;
    using sample_type = typename Traits::sample_type;

    scan_decoder_impl(const charls::frame_info& frame_info, const jpegls_pc_parameters& pc_parameters,
                      const coding_parameters& parameters, const Traits& traits) :
        scan_decoder{frame_info, pc_parameters, parameters}, traits_{traits}
    {
        ASSERT(traits_.is_valid());

        quantization_ = initialize_quantization_lut(traits_, t1_, t2_, t3_, quantization_lut_);
        initialize_parameters(traits_.range);
        copy_from_line_buffer_ = copy_from_line_buffer<sample_type>::get_copy_function(
            parameters.interleave_mode, frame_info.component_count, parameters.transformation);
    }

    [[nodiscard]]
    size_t decode_scan(const span<const std::byte> source, std::byte* destination, const size_t stride) override
    {
        const auto* scan_begin{to_address(source.begin())};

        initialize(source);

        // Process images without a restart interval, as 1 large restart interval.
        if (parameters_.restart_interval == 0)
        {
            parameters_.restart_interval = frame_info().height;
        }

        decode_lines(destination, stride);
        end_scan();

        return static_cast<size_t>(get_actual_position() - scan_begin);
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
    void decode_lines(std::byte* destination, const size_t stride)
    {
        const uint32_t pixel_stride{width_ + 2U};
        const size_t component_count{
            parameters().interleave_mode == interleave_mode::line ? static_cast<size_t>(frame_info().component_count) : 1U};

        std::array<uint32_t, maximum_component_count_in_scan> run_index{};
        std::vector<pixel_type> line_buffer(component_count * pixel_stride * 2);

        for (uint32_t line{};;)
        {
            const uint32_t lines_in_interval{std::min(frame_info().height - line, parameters_.restart_interval)};

            for (uint32_t mcu{}; mcu < lines_in_interval; ++mcu, ++line)
            {
                previous_line_ = line_buffer.data();
                current_line_ = line_buffer.data() + component_count * pixel_stride;
                if ((line & 1) == 1)
                {
                    std::swap(previous_line_, current_line_);
                }

                for (size_t component{}; component < component_count; ++component)
                {
                    run_index_ = run_index[component];

                    initialize_edge_pixels(previous_line_, current_line_, width_);

                    if constexpr (std::is_same_v<pixel_type, sample_type>)
                    {
                        decode_sample_line();
                    }
                    else if constexpr (std::is_same_v<pixel_type, pair<sample_type>>)
                    {
                        decode_pair_line();
                    }
                    else if constexpr (std::is_same_v<pixel_type, triplet<sample_type>>)
                    {
                        decode_triplet_line();
                    }
                    else
                    {
                        static_assert(std::is_same_v<pixel_type, quad<sample_type>>);
                        decode_quad_line();
                    }

                    run_index[component] = run_index_;
                    current_line_ += pixel_stride;
                    previous_line_ += pixel_stride;
                }

                copy_line_buffer_to_destination(current_line_ + 1 - (component_count * pixel_stride), destination, width_);
                destination += stride;
            }

            if (line == frame_info().height)
                break;

            process_restart_marker();

            // After a restart marker it is required to reset the decoder.
            std::fill(run_index.begin(), run_index.end(), 0);
            std::fill(line_buffer.begin(), line_buffer.end(), pixel_type{});
            initialize_parameters(traits_.range);
        }
    }

    /// <summary>Decodes a scan line of samples</summary>
    FORCE_INLINE void decode_sample_line()
    {
        size_t index{1};
        int32_t rb{*previous_line_};       // initial start value is rc, will be copied and overwritten in loop.
        int32_t rd{previous_line_[index]}; // initial start value is rb, will be copied and overwritten in loop.

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
                current_line_[index] = decode_regular(qs, compute_predicted_value(ra, rb, rc));
                ++index;
            }
            else
            {
                index += decode_run_mode(index);
                rb = previous_line_[index - 1];
                rd = previous_line_[index];
            }
        }
    }

    /// <summary>Decodes a scan line of triplets in ILV_SAMPLE mode</summary>
    void decode_pair_line()
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
                index += decode_run_mode(index);
            }
            else
            {
                pair<sample_type> rx;
                rx.v1 = decode_regular(qs1, compute_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = decode_regular(qs2, compute_predicted_value(ra.v2, rb.v2, rc.v2));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    /// <summary>Decodes a scan line of triplets in ILV_SAMPLE mode</summary>
    void decode_triplet_line()
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
                index += decode_run_mode(index);
            }
            else
            {
                triplet<sample_type> rx;
                rx.v1 = decode_regular(qs1, compute_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = decode_regular(qs2, compute_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = decode_regular(qs3, compute_predicted_value(ra.v3, rb.v3, rc.v3));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    /// <summary>Decodes a scan line of quads in ILV_SAMPLE mode</summary>
    void decode_quad_line()
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
                index += decode_run_mode(index);
            }
            else
            {
                quad<sample_type> rx;
                rx.v1 = decode_regular(qs1, compute_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = decode_regular(qs2, compute_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = decode_regular(qs3, compute_predicted_value(ra.v3, rb.v3, rc.v3));
                rx.v4 = decode_regular(qs4, compute_predicted_value(ra.v4, rb.v4, rc.v4));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    [[nodiscard]]
    size_t decode_run_mode(const size_t start_index)
    {
        const pixel_type ra{current_line_[start_index - 1]};

        const size_t run_length{decode_run_pixels(ra, current_line_ + start_index, width_ - (start_index - 1))};
        const auto end_index{static_cast<uint32_t>(start_index + run_length)};

        if (end_index - 1 == width_)
            return end_index - start_index;

        // Run interruption
        const pixel_type rb{previous_line_[end_index]};
        current_line_[end_index] = decode_run_interruption_pixel(ra, rb);
        decrement_run_index();
        return end_index - start_index + 1;
    }

    [[nodiscard]]
    FORCE_INLINE sample_type decode_regular(const int32_t qs, const int32_t predicted)
    {
        const int32_t sign{bit_wise_sign(qs)};
        regular_mode_context& context{regular_mode_contexts_[apply_sign_for_index(qs, sign)]};
        const int32_t corrected_prediction{traits_.correct_prediction(predicted + apply_sign(context.c(), sign))};
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
            error_value = unmap_error_value(decode_mapped_error_value(k, traits_.limit, traits_.quantized_bits_per_sample));
            if (UNLIKELY(std::abs(error_value) > 65535))
                impl::throw_jpegls_error(jpegls_errc::invalid_data);
        }

        if (k == 0)
        {
            error_value = error_value ^ context.get_error_correction(traits_.near_lossless);
        }

        context.update_variables_and_bias(error_value, traits_.near_lossless, reset_threshold_);
        error_value = apply_sign(error_value, sign);
        return traits_.compute_reconstructed_sample(corrected_prediction, error_value);
    }

    [[nodiscard]]
    int32_t decode_run_interruption_error(run_mode_context& context)
    {
        const int32_t k{context.compute_golomb_coding_parameter_checked()};
        const int32_t e_mapped_error_value{
            decode_mapped_error_value(k, traits_.limit - J[run_index_] - 1, traits_.quantized_bits_per_sample)};
        const int32_t error_value{context.compute_error_value(e_mapped_error_value + context.run_interruption_type(), k)};
        context.update_variables(error_value, e_mapped_error_value, reset_threshold_);
        return error_value;
    }

    [[nodiscard]]
    sample_type decode_run_interruption_pixel(int32_t ra, int32_t rb)
    {
        if (std::abs(ra - rb) <= traits_.near_lossless)
        {
            const int32_t error_value{decode_run_interruption_error(run_mode_contexts_[1])};
            return static_cast<sample_type>(traits_.compute_reconstructed_sample(ra, error_value));
        }

        const int32_t error_value{decode_run_interruption_error(run_mode_contexts_[0])};
        return static_cast<sample_type>(traits_.compute_reconstructed_sample(rb, error_value * sign(rb - ra)));
    }

    [[nodiscard]]
    pair<sample_type> decode_run_interruption_pixel(pair<sample_type> ra, pair<sample_type> rb)
    {
        const int32_t error_value1{decode_run_interruption_error(run_mode_contexts_[0])};
        const int32_t error_value2{decode_run_interruption_error(run_mode_contexts_[0])};

        return {traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2))};
    }

    [[nodiscard]]
    triplet<sample_type> decode_run_interruption_pixel(triplet<sample_type> ra, triplet<sample_type> rb)
    {
        const int32_t error_value1{decode_run_interruption_error(run_mode_contexts_[0])};
        const int32_t error_value2{decode_run_interruption_error(run_mode_contexts_[0])};
        const int32_t error_value3{decode_run_interruption_error(run_mode_contexts_[0])};

        return {traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3))};
    }

    [[nodiscard]]
    quad<sample_type> decode_run_interruption_pixel(quad<sample_type> ra, quad<sample_type> rb)
    {
        const int32_t error_value1{decode_run_interruption_error(run_mode_contexts_[0])};
        const int32_t error_value2{decode_run_interruption_error(run_mode_contexts_[0])};
        const int32_t error_value3{decode_run_interruption_error(run_mode_contexts_[0])};
        const int32_t error_value4{decode_run_interruption_error(run_mode_contexts_[0])};

        return {traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3)),
                traits_.compute_reconstructed_sample(rb.v4, error_value4 * sign(rb.v4 - ra.v4))};
    }

    [[nodiscard]]
    size_t decode_run_pixels(pixel_type ra, pixel_type* start_pos, const size_t pixel_count)
    {
        size_t index{};
        while (read_bit())
        {
            const size_t count{std::min(size_t{1} << J[run_index_], pixel_count - index)};
            index += count;
            ASSERT(index <= pixel_count);

            if (count == (size_t{1} << J[run_index_]))
            {
                increment_run_index();
            }

            if (index == pixel_count)
                break;
        }

        if (index != pixel_count)
        {
            // Incomplete run.
            index += (J[run_index_] > 0) ? read_value(J[run_index_]) : 0;
        }

        if (UNLIKELY(index > pixel_count))
            impl::throw_jpegls_error(jpegls_errc::invalid_data);

        for (size_t i{}; i < index; ++i)
        {
            start_pos[i] = ra;
        }

        return index;
    }

    Traits traits_;
    pixel_type* previous_line_{};
    pixel_type* current_line_{};
};

} // namespace charls
