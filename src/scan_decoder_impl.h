// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "color_transform.h"
#include "golomb_lut.h"
#include "scan_decoder.h"

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
        reset_parameters(traits_.range);
    }

    size_t decode_scan(const span<const std::byte> source, std::byte* destination, const size_t stride) override
    {
        process_line_ = create_process_line(destination, stride);

        const auto* scan_begin{to_address(source.begin())};

        initialize(source);

        // Process images without a restart interval, as 1 large restart interval.
        if (parameters_.restart_interval == 0)
        {
            parameters_.restart_interval = frame_info().height;
        }

        decode_lines();
        end_scan();

        return get_cur_byte_pos() - scan_begin;
    }

private:
    // Factory function for ProcessLine objects to copy/transform un encoded pixels to/from our scan line buffers.
    std::unique_ptr<process_decoded_line> create_process_line(std::byte* destination, const size_t stride)
    {
        if (parameters().interleave_mode == interleave_mode::none)
        {
            return std::make_unique<process_decoded_single_component>(destination, stride, sizeof(pixel_type));
        }

        switch (parameters().transformation)
        {
        case color_transformation::none:
            return std::make_unique<process_decoded_transformed<transform_none<sample_type>>>(
                destination, stride, frame_info().component_count, parameters().interleave_mode);
        case color_transformation::hp1:
            ASSERT(color_transformation_possible(frame_info()));
            return std::make_unique<process_decoded_transformed<transform_hp1<sample_type>>>(
                destination, stride, frame_info().component_count, parameters().interleave_mode);
        case color_transformation::hp2:
            ASSERT(color_transformation_possible(frame_info()));
            return std::make_unique<process_decoded_transformed<transform_hp2<sample_type>>>(
                destination, stride, frame_info().component_count, parameters().interleave_mode);
        case color_transformation::hp3:
            ASSERT(color_transformation_possible(frame_info()));
            return std::make_unique<process_decoded_transformed<transform_hp3<sample_type>>>(
                destination, stride, frame_info().component_count, parameters().interleave_mode);
        }

        unreachable();
    }

    [[nodiscard]]
    FORCE_INLINE int32_t quantize_gradient(const int32_t di) const noexcept
    {
        ASSERT(quantize_gradient_org(di, traits_.near_lossless) == *(quantization_ + di));
        return *(quantization_ + di);
    }

    // In ILV_SAMPLE mode, multiple components are handled in do_line
    // In ILV_LINE mode, a call to do_line is made for every component
    // In ILV_NONE mode, do_scan is called for each component
    void decode_lines()
    {
        const uint32_t pixel_stride{width_ + 4U};
        const size_t component_count{
            parameters().interleave_mode == interleave_mode::line ? static_cast<size_t>(frame_info().component_count) : 1U};
        uint32_t restart_interval_counter{};

        std::vector<pixel_type> line_buffer(component_count * pixel_stride * 2);
        std::vector<int32_t> run_index(component_count);

        for (uint32_t line{};;)
        {
            const uint32_t lines_in_interval{std::min(frame_info().height - line, parameters_.restart_interval)};

            for (uint32_t mcu{}; mcu < lines_in_interval; ++mcu, ++line)
            {
                previous_line_ = &line_buffer[1];
                current_line_ = &line_buffer[1 + component_count * pixel_stride];
                if ((line & 1) == 1)
                {
                    std::swap(previous_line_, current_line_);
                }

                for (size_t component{}; component < component_count; ++component)
                {
                    run_index_ = run_index[component];

                    // initialize edge pixels used for prediction
                    previous_line_[width_] = previous_line_[width_ - 1];
                    current_line_[-1] = previous_line_[0];

                    if constexpr (std::is_same_v<pixel_type, sample_type>)
                    {
                        decode_sample_line();
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
                    previous_line_ += pixel_stride;
                    current_line_ += pixel_stride;
                }

                on_line_end(current_line_ - (component_count * pixel_stride), frame_info().width, pixel_stride);
            }

            if (line == frame_info().height)
                break;

            // At this point in the byte stream a restart marker should be present: process it.
            read_restart_marker(restart_interval_counter);
            restart_interval_counter = (restart_interval_counter + 1) % jpeg_restart_marker_range;

            // After a restart marker it is required to reset the decoder.
            reset();
            std::fill(line_buffer.begin(), line_buffer.end(), pixel_type{});
            std::fill(run_index.begin(), run_index.end(), 0);
            reset_parameters(traits_.range);
        }
    }

    /// <summary>Decodes a scan line of samples</summary>
    FORCE_INLINE void decode_sample_line()
    {
        int32_t index{};
        int32_t rb{previous_line_[index - 1]};
        int32_t rd{previous_line_[index]};

        while (static_cast<uint32_t>(index) < width_)
        {
            const int32_t ra{current_line_[index - 1]};
            const int32_t rc{rb};
            rb = rd;
            rd = previous_line_[index + 1];

            if (const int32_t qs{
                    compute_context_id(quantize_gradient(rd - rb), quantize_gradient(rb - rc), quantize_gradient(rc - ra))};
                qs != 0)
            {
                current_line_[index] = decode_regular(qs, get_predicted_value(ra, rb, rc));
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
    void decode_triplet_line()
    {
        int32_t index{};
        while (static_cast<uint32_t>(index) < width_)
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
                rx.v1 = decode_regular(qs1, get_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = decode_regular(qs2, get_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = decode_regular(qs3, get_predicted_value(ra.v3, rb.v3, rc.v3));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    /// <summary>Decodes a scan line of quads in ILV_SAMPLE mode</summary>
    void decode_quad_line()
    {
        int32_t index{};
        while (static_cast<uint32_t>(index) < width_)
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
                rx.v1 = decode_regular(qs1, get_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = decode_regular(qs2, get_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = decode_regular(qs3, get_predicted_value(ra.v3, rb.v3, rc.v3));
                rx.v4 = decode_regular(qs4, get_predicted_value(ra.v4, rb.v4, rc.v4));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    [[nodiscard]]
    int32_t decode_run_mode(const int32_t start_index)
    {
        const pixel_type ra{current_line_[start_index - 1]};

        const int32_t run_length{decode_run_pixels(ra, current_line_ + start_index, width_ - start_index)};
        const auto end_index{static_cast<uint32_t>(start_index + run_length)};

        if (end_index == width_)
            return end_index - start_index;

        // run interruption
        const pixel_type rb{previous_line_[end_index]};
        current_line_[end_index] = decode_run_interruption_pixel(ra, rb);
        decrement_run_index();
        return end_index - start_index + 1;
    }

    [[nodiscard]]
    FORCE_INLINE sample_type decode_regular(const int32_t qs, const int32_t predicted)
    {
        const int32_t sign{bit_wise_sign(qs)};
        context_regular_mode& context{contexts_[apply_sign(qs, sign)]};
        const int32_t k{context.get_golomb_coding_parameter()};
        const int32_t predicted_value{traits_.correct_prediction(predicted + apply_sign(context.c(), sign))};

        int32_t error_value;
        if (const golomb_code& code = golomb_lut[k].get(peek_byte()); code.length() != 0)
        {
            skip(code.length());
            error_value = code.value();
            ASSERT(std::abs(error_value) < 65535);
        }
        else
        {
            error_value = unmap_error_value(decode_value(k, traits_.limit, traits_.quantized_bits_per_pixel));
            if (UNLIKELY(std::abs(error_value) > 65535))
                impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);
        }
        if (k == 0)
        {
            error_value = error_value ^ context.get_error_correction(traits_.near_lossless);
        }
        context.update_variables_and_bias(error_value, traits_.near_lossless, traits_.reset_threshold);
        error_value = apply_sign(error_value, sign);
        return traits_.compute_reconstructed_sample(predicted_value, error_value);
    }

    [[nodiscard]]
    int32_t decode_run_interruption_error(context_run_mode& context)
    {
        const int32_t k{context.get_golomb_code()};
        const int32_t e_mapped_error_value{
            decode_value(k, traits_.limit - J[run_index_] - 1, traits_.quantized_bits_per_pixel)};
        const int32_t error_value{context.compute_error_value(e_mapped_error_value + context.run_interruption_type(), k)};
        context.update_variables(error_value, e_mapped_error_value, reset_value_);
        return error_value;
    }

    [[nodiscard]]
    triplet<sample_type> decode_run_interruption_pixel(triplet<sample_type> ra, triplet<sample_type> rb)
    {
        const int32_t error_value1{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value2{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value3{decode_run_interruption_error(context_run_mode_[0])};

        return {traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3))};
    }

    [[nodiscard]]
    quad<sample_type> decode_run_interruption_pixel(quad<sample_type> ra, quad<sample_type> rb)
    {
        const int32_t error_value1{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value2{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value3{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value4{decode_run_interruption_error(context_run_mode_[0])};

        return {traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3)),
                traits_.compute_reconstructed_sample(rb.v4, error_value4 * sign(rb.v4 - ra.v4))};
    }

    [[nodiscard]]
    sample_type decode_run_interruption_pixel(int32_t ra, int32_t rb)
    {
        if (std::abs(ra - rb) <= traits_.near_lossless)
        {
            const int32_t error_value{decode_run_interruption_error(context_run_mode_[1])};
            return static_cast<sample_type>(traits_.compute_reconstructed_sample(ra, error_value));
        }

        const int32_t error_value{decode_run_interruption_error(context_run_mode_[0])};
        return static_cast<sample_type>(traits_.compute_reconstructed_sample(rb, error_value * sign(rb - ra)));
    }

    [[nodiscard]]
    int32_t decode_run_pixels(pixel_type ra, pixel_type* start_pos, const int32_t pixel_count)
    {
        int32_t index{};
        while (read_bit())
        {
            const int count{std::min(1 << J[run_index_], pixel_count - index)};
            index += count;
            ASSERT(index <= pixel_count);

            if (count == (1 << J[run_index_]))
            {
                increment_run_index();
            }

            if (index == pixel_count)
                break;
        }

        if (index != pixel_count)
        {
            // incomplete run.
            index += (J[run_index_] > 0) ? read_value(J[run_index_]) : 0;
        }

        if (UNLIKELY(index > pixel_count))
            impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);

        for (int32_t i{}; i < index; ++i)
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
