// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "coding_parameters.h"
#include "color_transform.h"
#include "context_regular_mode.h"
#include "context_run_mode.h"
#include "encoder_strategy.h"
#include "decoder_strategy.h"
#include "jpeg_marker_code.h"
#include "process_line.h"
#include "jpegls_algorithm.h"
#include "golomb_lut.h"


// This file contains the code for handling a "scan". Usually an image is encoded as a single scan.
// Note: the functions in this header could be moved into jpegls.cpp as they are only used in that file.

namespace charls {

extern const std::vector<int8_t> quantization_lut_lossless_8;
extern const std::vector<int8_t> quantization_lut_lossless_10;
extern const std::vector<int8_t> quantization_lut_lossless_12;
extern const std::vector<int8_t> quantization_lut_lossless_16;


template<typename Traits>
class scan_encoder_implementation final : public encoder_strategy
{
public:
    using pixel_type = typename Traits::pixel_type;
    using sample_type = typename Traits::sample_type;

    scan_encoder_implementation(Traits traits, const charls::frame_info& frame_info, const coding_parameters& parameters) noexcept :
        encoder_strategy{update_component_count(frame_info, parameters), parameters},
        traits_{std::move(traits)},
        width_{frame_info.width}
    {
        ASSERT((parameters.interleave_mode == interleave_mode::none && this->frame_info().component_count == 1) ||
               parameters.interleave_mode != interleave_mode::none);
        ASSERT(traits_.is_valid());
    }

private:
    // Factory function for ProcessLine objects to copy/transform un encoded pixels to/from our scan line buffers.
    std::unique_ptr<process_line> create_process_line(const_byte_span source, const size_t stride)
    {
        if (!is_interleaved())
        {
            if (frame_info().bits_per_sample == sizeof(sample_type) * 8)
            {
                return std::make_unique<post_process_single_component>(source.data(), stride,
                                                                       sizeof(typename Traits::pixel_type));
            }

            return std::make_unique<post_process_single_component_masked>(
                source.data(), stride, sizeof(typename Traits::pixel_type), frame_info().bits_per_sample);
        }

        if (parameters().transformation == color_transformation::none)
            return std::make_unique<process_transformed<transform_none<typename Traits::sample_type>>>(
                source, stride, frame_info(), parameters(), transform_none<sample_type>());

        if (frame_info().bits_per_sample == sizeof(sample_type) * 8)
        {
            switch (parameters().transformation)
            {
            case color_transformation::hp1:
                return std::make_unique<process_transformed<transform_hp1<sample_type>>>(
                    source, stride, frame_info(), parameters(), transform_hp1<sample_type>());
            case color_transformation::hp2:
                return std::make_unique<process_transformed<transform_hp2<sample_type>>>(
                    source, stride, frame_info(), parameters(), transform_hp2<sample_type>());
            case color_transformation::hp3:
                return std::make_unique<process_transformed<transform_hp3<sample_type>>>(
                    source, stride, frame_info(), parameters(), transform_hp3<sample_type>());
            default:
                impl::throw_jpegls_error(jpegls_errc::color_transform_not_supported);
            }
        }

        impl::throw_jpegls_error(jpegls_errc::bit_depth_for_transform_not_supported);
    }

    void set_presets(const jpegls_pc_parameters& presets) override
    {
        initialize_parameters(presets.threshold1, presets.threshold2, presets.threshold3, presets.reset_value);
    }

    [[nodiscard]] FORCE_INLINE int32_t quantize_gradient(const int32_t di) const noexcept
    {
        ASSERT(quantize_gradient_org(di, traits_.near_lossless) == *(quantization_ + di));
        return *(quantization_ + di);
    }

    // C4127 = conditional expression is constant (caused by some template methods that are not fully specialized) [VS2017]
    // C6326 = Potential comparison of a constant with another constant. (false warning, triggered by template construction
    // in Checked build)
    // C26814 = The const variable 'RANGE' can be computed at compile-time. [incorrect warning, VS 16.3.0 P3]
    MSVC_WARNING_SUPPRESS(4127 6326 26814)

    void initialize_quantization_lut()
    {
        // for lossless mode with default parameters, we have precomputed the look up table for bit counts 8, 10, 12 and 16.
        if (traits_.near_lossless == 0 && traits_.maximum_sample_value == (1 << traits_.bits_per_pixel) - 1)
        {
            if (const jpegls_pc_parameters presets{compute_default(traits_.maximum_sample_value, traits_.near_lossless)};
                presets.threshold1 == t1_ && presets.threshold2 == t2_ && presets.threshold3 == t3_)
            {
                if (traits_.bits_per_pixel == 8)
                {
                    quantization_ = &quantization_lut_lossless_8[quantization_lut_lossless_8.size() / 2];
                    return;
                }
                if (traits_.bits_per_pixel == 10)
                {
                    quantization_ = &quantization_lut_lossless_10[quantization_lut_lossless_10.size() / 2];
                    return;
                }
                if (traits_.bits_per_pixel == 12)
                {
                    quantization_ = &quantization_lut_lossless_12[quantization_lut_lossless_12.size() / 2];
                    return;
                }
                if (traits_.bits_per_pixel == 16)
                {
                    quantization_ = &quantization_lut_lossless_16[quantization_lut_lossless_16.size() / 2];
                    return;
                }
            }
        }

        // Initialize the quantization lookup table dynamic.
        const int32_t range{1 << traits_.bits_per_pixel};
        quantization_lut_.resize(static_cast<size_t>(range) * 2);
        for (size_t i{}; i < quantization_lut_.size(); ++i)
        {
            quantization_lut_[i] = quantize_gradient_org(-range + static_cast<int32_t>(i), traits_.near_lossless);
        }

        quantization_ = &quantization_lut_[range];
    }
    MSVC_WARNING_UNSUPPRESS()

    FORCE_INLINE void encode_mapped_value(const int32_t k, const int32_t mapped_error, const int32_t limit)
    {
        if (int32_t high_bits{mapped_error >> k}; high_bits < limit - traits_.quantized_bits_per_pixel - 1)
        {
            if (high_bits + 1 > 31)
            {
                append_to_bit_stream(0, high_bits / 2);
                high_bits = high_bits - high_bits / 2;
            }
            append_to_bit_stream(1, high_bits + 1);
            append_to_bit_stream((mapped_error & ((1 << k) - 1)), k);
            return;
        }

        if (limit - traits_.quantized_bits_per_pixel > 31)
        {
            append_to_bit_stream(0, 31);
            append_to_bit_stream(1, limit - traits_.quantized_bits_per_pixel - 31);
        }
        else
        {
            append_to_bit_stream(1, limit - traits_.quantized_bits_per_pixel);
        }
        append_to_bit_stream((mapped_error - 1) & ((1 << traits_.quantized_bits_per_pixel) - 1),
                             traits_.quantized_bits_per_pixel);
    }

    void increment_run_index() noexcept
    {
        run_index_ = std::min(31, run_index_ + 1);
    }

    void decrement_run_index() noexcept
    {
        run_index_ = std::max(0, run_index_ - 1);
    }

    FORCE_INLINE sample_type do_regular(const int32_t qs, const int32_t x, const int32_t predicted)
    {
        const int32_t sign{bit_wise_sign(qs)};
        context_regular_mode& context{contexts_[apply_sign(qs, sign)]};
        const int32_t k{context.get_golomb_coding_parameter()};
        const int32_t predicted_value{traits_.correct_prediction(predicted + apply_sign(context.c(), sign))};
        const int32_t error_value{traits_.compute_error_value(apply_sign(x - predicted_value, sign))};

        encode_mapped_value(k, map_error_value(context.get_error_correction(k | traits_.near_lossless) ^ error_value),
                            traits_.limit);
        context.update_variables_and_bias(error_value, traits_.near_lossless, traits_.reset_threshold);
        ASSERT(traits_.is_near(traits_.compute_reconstructed_sample(predicted_value, apply_sign(error_value, sign)), x));
        return static_cast<sample_type>(
            traits_.compute_reconstructed_sample(predicted_value, apply_sign(error_value, sign)));
    }

    /// <summary>Encodes a scan line of samples</summary>
    FORCE_INLINE void do_line(sample_type* /*template_selector*/)
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
                current_line_[index] =
                    do_regular(qs, current_line_[index], get_predicted_value(ra, rb, rc));
                ++index;
            }
            else
            {
                index += do_run_mode(index);
                rb = previous_line_[index - 1];
                rd = previous_line_[index];
            }
        }
    }

    /// <summary>Encodes a scan line of triplets in ILV_SAMPLE mode</summary>
    void do_line(triplet<sample_type>* /*template_selector*/)
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
                index += do_run_mode(index);
            }
            else
            {
                triplet<sample_type> rx;
                rx.v1 = do_regular(qs1, current_line_[index].v1, get_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = do_regular(qs2, current_line_[index].v2, get_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = do_regular(qs3, current_line_[index].v3, get_predicted_value(ra.v3, rb.v3, rc.v3));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    /// <summary>Encodes a scan line of quads in ILV_SAMPLE mode</summary>
    void do_line(quad<sample_type>* /*template_selector*/)
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
                index += do_run_mode(index);
            }
            else
            {
                quad<sample_type> rx;
                rx.v1 = do_regular(qs1, current_line_[index].v1, get_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = do_regular(qs2, current_line_[index].v2, get_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = do_regular(qs3, current_line_[index].v3, get_predicted_value(ra.v3, rb.v3, rc.v3));
                rx.v4 = do_regular(qs4, current_line_[index].v4, get_predicted_value(ra.v4, rb.v4, rc.v4));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    size_t encode_scan(const const_byte_span source, const size_t stride, const byte_span destination) override
    {
        process_line_ = create_process_line(source, stride);

        initialize(destination);
        encode_lines();

        return get_length();
    }

    void initialize_parameters(const int32_t t1, const int32_t t2, const int32_t t3, const int32_t reset_threshold)
    {
        t1_ = t1;
        t2_ = t2;
        t3_ = t3;
        reset_threshold_ = static_cast<uint8_t>(reset_threshold);

        initialize_quantization_lut();
        reset_parameters();
    }

    void reset_parameters() noexcept
    {
        const context_regular_mode context_initial_value(traits_.range);
        for (auto& context : contexts_)
        {
            context = context_initial_value;
        }

        context_run_mode_[0] = context_run_mode(0, traits_.range);
        context_run_mode_[1] = context_run_mode(1, traits_.range);
        run_index_ = 0;
    }

    static charls::frame_info update_component_count(charls::frame_info frame, const coding_parameters& parameters) noexcept
    {
        if (parameters.interleave_mode == interleave_mode::none)
        {
            frame.component_count = 1;
        }

        return frame;
    }

    // In ILV_SAMPLE mode, multiple components are handled in do_line
    // In ILV_LINE mode, a call to do_line is made for every component
    // In ILV_NONE mode, do_scan is called for each component
    void encode_lines()
    {
        const uint32_t pixel_stride{width_ + 4U};
        const size_t component_count{
            parameters().interleave_mode == interleave_mode::line ? static_cast<size_t>(frame_info().component_count) : 1U};

        std::vector<pixel_type> line_buffer(component_count * pixel_stride * 2);
        std::vector<int32_t> run_index(component_count);

        for (uint32_t line{}; line < frame_info().height; ++line)
        {
            previous_line_ = &line_buffer[1];
            current_line_ = &line_buffer[1 + static_cast<size_t>(component_count) * pixel_stride];
            if ((line & 1) == 1)
            {
                std::swap(previous_line_, current_line_);
            }

            on_line_begin(current_line_, width_, pixel_stride);

            for (size_t component{}; component < component_count; ++component)
            {
                run_index_ = run_index[component];

                // initialize edge pixels used for prediction
                previous_line_[width_] = previous_line_[width_ - 1];
                current_line_[-1] = previous_line_[0];
                do_line(static_cast<pixel_type*>(nullptr)); // dummy argument for overload resolution

                run_index[component] = run_index_;
                previous_line_ += pixel_stride;
                current_line_ += pixel_stride;
            }
        }

        end_scan();
    }

    void encode_run_interruption_error(context_run_mode& context, const int32_t error_value)
    {
        const int32_t k{context.get_golomb_code()};
        const bool map{context.compute_map(error_value, k)};
        const int32_t e_mapped_error_value{2 * std::abs(error_value) - context.run_interruption_type() -
                                           static_cast<int32_t>(map)};

        ASSERT(error_value == context.compute_error_value(e_mapped_error_value + context.run_interruption_type(), k));
        encode_mapped_value(k, e_mapped_error_value, traits_.limit - J[run_index_] - 1);
        context.update_variables(error_value, e_mapped_error_value, reset_threshold_);
    }

    sample_type encode_run_interruption_pixel(const int32_t x, const int32_t ra, const int32_t rb)
    {
        if (std::abs(ra - rb) <= traits_.near_lossless)
        {
            const int32_t error_value{traits_.compute_error_value(x - ra)};
            encode_run_interruption_error(context_run_mode_[1], error_value);
            return static_cast<sample_type>(traits_.compute_reconstructed_sample(ra, error_value));
        }

        const int32_t error_value{traits_.compute_error_value((x - rb) * sign(rb - ra))};
        encode_run_interruption_error(context_run_mode_[0], error_value);
        return static_cast<sample_type>(traits_.compute_reconstructed_sample(rb, error_value * sign(rb - ra)));
    }

    triplet<sample_type> encode_run_interruption_pixel(const triplet<sample_type> x, const triplet<sample_type> ra,
                                                       const triplet<sample_type> rb)
    {
        const int32_t error_value1{traits_.compute_error_value(sign(rb.v1 - ra.v1) * (x.v1 - rb.v1))};
        encode_run_interruption_error(context_run_mode_[0], error_value1);

        const int32_t error_value2{traits_.compute_error_value(sign(rb.v2 - ra.v2) * (x.v2 - rb.v2))};
        encode_run_interruption_error(context_run_mode_[0], error_value2);

        const int32_t error_value3{traits_.compute_error_value(sign(rb.v3 - ra.v3) * (x.v3 - rb.v3))};
        encode_run_interruption_error(context_run_mode_[0], error_value3);

        return triplet<sample_type>(traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                                    traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                                    traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3)));
    }

    quad<sample_type> encode_run_interruption_pixel(const quad<sample_type> x, const quad<sample_type> ra,
                                                    const quad<sample_type> rb)
    {
        const int32_t error_value1{traits_.compute_error_value(sign(rb.v1 - ra.v1) * (x.v1 - rb.v1))};
        encode_run_interruption_error(context_run_mode_[0], error_value1);

        const int32_t error_value2{traits_.compute_error_value(sign(rb.v2 - ra.v2) * (x.v2 - rb.v2))};
        encode_run_interruption_error(context_run_mode_[0], error_value2);

        const int32_t error_value3{traits_.compute_error_value(sign(rb.v3 - ra.v3) * (x.v3 - rb.v3))};
        encode_run_interruption_error(context_run_mode_[0], error_value3);

        const int32_t error_value4{traits_.compute_error_value(sign(rb.v4 - ra.v4) * (x.v4 - rb.v4))};
        encode_run_interruption_error(context_run_mode_[0], error_value4);

        return quad<sample_type>(
            triplet<sample_type>(traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                                 traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                                 traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3))),
            traits_.compute_reconstructed_sample(rb.v4, error_value4 * sign(rb.v4 - ra.v4)));
    }

    void encode_run_pixels(int32_t run_length, const bool end_of_line)
    {
        while (run_length >= 1 << J[run_index_])
        {
            append_ones_to_bit_stream(1);
            run_length = run_length - (1 << J[run_index_]);
            increment_run_index();
        }

        if (end_of_line)
        {
            if (run_length != 0)
            {
                append_ones_to_bit_stream(1);
            }
        }
        else
        {
            append_to_bit_stream(run_length, J[run_index_] + 1); // leading 0 + actual remaining length
        }
    }

    int32_t do_run_mode(const int32_t index)
    {
        const int32_t count_type_remain = width_ - index;
        pixel_type* type_cur_x{current_line_ + index};
        const pixel_type* type_prev_x{previous_line_ + index};

        const pixel_type ra{type_cur_x[-1]};

        int32_t run_length{};
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

    // codec parameters
    Traits traits_;
    uint32_t width_;
    uint8_t reset_threshold_{};

    // compression context
    std::array<context_regular_mode, 365> contexts_;
    std::array<context_run_mode, 2> context_run_mode_;
    int32_t run_index_{};
    pixel_type* previous_line_{};
    pixel_type* current_line_{};

    // quantization lookup table
    const int8_t* quantization_{};
    std::vector<int8_t> quantization_lut_;
};


template<typename Traits>
class scan_decoder_implementation final : public decoder_strategy
{
public:
    using pixel_type = typename Traits::pixel_type;
    using sample_type = typename Traits::sample_type;

    scan_decoder_implementation(Traits traits, const charls::frame_info& frame_info, const coding_parameters& parameters) noexcept :
        decoder_strategy{update_component_count(frame_info, parameters), parameters},
        traits_{std::move(traits)},
        width_{frame_info.width}
    {
        ASSERT((parameters.interleave_mode == interleave_mode::none && this->frame_info().component_count == 1) ||
               parameters.interleave_mode != interleave_mode::none);
        ASSERT(traits_.is_valid());
    }

private:
    // Factory function for ProcessLine objects to copy/transform un encoded pixels to/from our scan line buffers.
    std::unique_ptr<process_line> create_process_line(byte_span destination, const size_t stride)
    {
        if (!is_interleaved())
        {
            if (frame_info().bits_per_sample == sizeof(sample_type) * 8)
            {
                return std::make_unique<post_process_single_component>(destination.data(), stride,
                                                                       sizeof(typename Traits::pixel_type));
            }

            return std::make_unique<post_process_single_component_masked>(
                destination.data(), stride, sizeof(typename Traits::pixel_type), frame_info().bits_per_sample);
        }

        if (parameters().transformation == color_transformation::none)
            return std::make_unique<process_transformed<transform_none<typename Traits::sample_type>>>(
                destination, stride, frame_info(), parameters(), transform_none<sample_type>());

        if (frame_info().bits_per_sample == sizeof(sample_type) * 8)
        {
            switch (parameters().transformation)
            {
            case color_transformation::hp1:
                return std::make_unique<process_transformed<transform_hp1<sample_type>>>(
                    destination, stride, frame_info(), parameters(), transform_hp1<sample_type>());
            case color_transformation::hp2:
                return std::make_unique<process_transformed<transform_hp2<sample_type>>>(
                    destination, stride, frame_info(), parameters(), transform_hp2<sample_type>());
            case color_transformation::hp3:
                return std::make_unique<process_transformed<transform_hp3<sample_type>>>(
                    destination, stride, frame_info(), parameters(), transform_hp3<sample_type>());
            default:
                impl::throw_jpegls_error(jpegls_errc::color_transform_not_supported);
            }
        }

        impl::throw_jpegls_error(jpegls_errc::bit_depth_for_transform_not_supported);
    }

    void set_presets(const jpegls_pc_parameters& presets) override
    {
        initialize_parameters(presets.threshold1, presets.threshold2, presets.threshold3, presets.reset_value);
    }

    [[nodiscard]] FORCE_INLINE int32_t quantize_gradient(const int32_t di) const noexcept
    {
        ASSERT(quantize_gradient_org(di, traits_.near_lossless) == *(quantization_ + di));
        return *(quantization_ + di);
    }

    // C4127 = conditional expression is constant (caused by some template methods that are not fully specialized) [VS2017]
    // C6326 = Potential comparison of a constant with another constant. (false warning, triggered by template construction
    // in Checked build)
    // C26814 = The const variable 'RANGE' can be computed at compile-time. [incorrect warning, VS 16.3.0 P3]
    MSVC_WARNING_SUPPRESS(4127 6326 26814)

    void initialize_quantization_lut()
    {
        // for lossless mode with default parameters, we have precomputed the look up table for bit counts 8, 10, 12 and 16.
        if (traits_.near_lossless == 0 && traits_.maximum_sample_value == (1 << traits_.bits_per_pixel) - 1)
        {
            if (const jpegls_pc_parameters presets{compute_default(traits_.maximum_sample_value, traits_.near_lossless)};
                presets.threshold1 == t1_ && presets.threshold2 == t2_ && presets.threshold3 == t3_)
            {
                if (traits_.bits_per_pixel == 8)
                {
                    quantization_ = &quantization_lut_lossless_8[quantization_lut_lossless_8.size() / 2];
                    return;
                }
                if (traits_.bits_per_pixel == 10)
                {
                    quantization_ = &quantization_lut_lossless_10[quantization_lut_lossless_10.size() / 2];
                    return;
                }
                if (traits_.bits_per_pixel == 12)
                {
                    quantization_ = &quantization_lut_lossless_12[quantization_lut_lossless_12.size() / 2];
                    return;
                }
                if (traits_.bits_per_pixel == 16)
                {
                    quantization_ = &quantization_lut_lossless_16[quantization_lut_lossless_16.size() / 2];
                    return;
                }
            }
        }

        // Initialize the quantization lookup table dynamic.
        const int32_t range{1 << traits_.bits_per_pixel};
        quantization_lut_.resize(static_cast<size_t>(range) * 2);
        for (size_t i{}; i < quantization_lut_.size(); ++i)
        {
            quantization_lut_[i] = quantize_gradient_org(-range + static_cast<int32_t>(i), traits_.near_lossless);
        }

        quantization_ = &quantization_lut_[range];
    }
    MSVC_WARNING_UNSUPPRESS()

    int32_t decode_value(const int32_t k, const int32_t limit, const int32_t quantized_bits_per_pixel)
    {
        const int32_t high_bits{read_high_bits()};

        if (high_bits >= limit - (quantized_bits_per_pixel + 1))
            return read_value(quantized_bits_per_pixel) + 1;

        if (k == 0)
            return high_bits;

        return (high_bits << k) + read_value(k);
    }

    void increment_run_index() noexcept
    {
        run_index_ = std::min(31, run_index_ + 1);
    }

    void decrement_run_index() noexcept
    {
        run_index_ = std::max(0, run_index_ - 1);
    }

    FORCE_INLINE sample_type do_regular(const int32_t qs, int32_t /*x*/, const int32_t predicted)
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

    /// <summary>Decodes a scan line of samples</summary>
    FORCE_INLINE void do_line(sample_type* /*template_selector*/)
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
                current_line_[index] = do_regular(qs, current_line_[index], get_predicted_value(ra, rb, rc));
                ++index;
            }
            else
            {
                index += do_run_mode(index);
                rb = previous_line_[index - 1];
                rd = previous_line_[index];
            }
        }
    }

    /// <summary>Decodes a scan line of triplets in ILV_SAMPLE mode</summary>
    void do_line(triplet<sample_type>* /*template_selector*/)
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
                index += do_run_mode(index);
            }
            else
            {
                triplet<sample_type> rx;
                rx.v1 = do_regular(qs1, current_line_[index].v1, get_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = do_regular(qs2, current_line_[index].v2, get_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = do_regular(qs3, current_line_[index].v3, get_predicted_value(ra.v3, rb.v3, rc.v3));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    /// <summary>Decodes a scan line of quads in ILV_SAMPLE mode</summary>
    void do_line(quad<sample_type>* /*template_selector*/)
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
                index += do_run_mode(index);
            }
            else
            {
                quad<sample_type> rx;
                rx.v1 = do_regular(qs1, current_line_[index].v1, get_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = do_regular(qs2, current_line_[index].v2, get_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = do_regular(qs3, current_line_[index].v3, get_predicted_value(ra.v3, rb.v3, rc.v3));
                rx.v4 = do_regular(qs4, current_line_[index].v4, get_predicted_value(ra.v4, rb.v4, rc.v4));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    size_t decode_scan(const const_byte_span source, const byte_span destination, const size_t stride) override
    {
        process_line_ = create_process_line(destination, stride);

        const auto* scan_begin{source.begin()};

        initialize(source);

        // Process images without a restart interval, as 1 large restart interval.b
        if (parameters_.restart_interval == 0)
        {
            parameters_.restart_interval = frame_info().height;
        }

        decode_lines();

        return get_cur_byte_pos() - scan_begin;
    }

    void initialize_parameters(const int32_t t1, const int32_t t2, const int32_t t3, const int32_t reset_threshold)
    {
        t1_ = t1;
        t2_ = t2;
        t3_ = t3;
        reset_threshold_ = static_cast<uint8_t>(reset_threshold);

        initialize_quantization_lut();
        reset_parameters();
    }

    void reset_parameters() noexcept
    {
        const context_regular_mode context_initial_value(traits_.range);
        for (auto& context : contexts_)
        {
            context = context_initial_value;
        }

        context_run_mode_[0] = context_run_mode(0, traits_.range);
        context_run_mode_[1] = context_run_mode(1, traits_.range);
        run_index_ = 0;
    }

    static charls::frame_info update_component_count(charls::frame_info frame, const coding_parameters& parameters) noexcept
    {
        if (parameters.interleave_mode == interleave_mode::none)
        {
            frame.component_count = 1;
        }

        return frame;
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
                    do_line(static_cast<pixel_type*>(nullptr)); // dummy argument for overload resolution

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
            reset_parameters();
        }

        end_scan();
    }

    void read_restart_marker(const uint32_t expected_restart_marker_id)
    {
        auto value{read_byte()};
        if (UNLIKELY(value != jpeg_marker_start_byte))
            impl::throw_jpegls_error(jpegls_errc::restart_marker_not_found);

        // Read all preceding 0xFF fill bytes until a non 0xFF byte has been found. (see T.81, B.1.1.2)
        do
        {
            value = read_byte();
        } while (value == jpeg_marker_start_byte);

        if (UNLIKELY(std::to_integer<uint32_t>(value) != jpeg_restart_marker_base + expected_restart_marker_id))
            impl::throw_jpegls_error(jpegls_errc::restart_marker_not_found);
    }

    int32_t decode_run_interruption_error(context_run_mode& context)
    {
        const int32_t k{context.get_golomb_code()};
        const int32_t e_mapped_error_value{
            decode_value(k, traits_.limit - J[run_index_] - 1, traits_.quantized_bits_per_pixel)};
        const int32_t error_value{context.compute_error_value(e_mapped_error_value + context.run_interruption_type(), k)};
        context.update_variables(error_value, e_mapped_error_value, reset_threshold_);
        return error_value;
    }

    triplet<sample_type> decode_run_interruption_pixel(triplet<sample_type> ra, triplet<sample_type> rb)
    {
        const int32_t error_value1{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value2{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value3{decode_run_interruption_error(context_run_mode_[0])};

        return triplet<sample_type>(traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                                    traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                                    traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3)));
    }

    quad<sample_type> decode_run_interruption_pixel(quad<sample_type> ra, quad<sample_type> rb)
    {
        const int32_t error_value1{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value2{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value3{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value4{decode_run_interruption_error(context_run_mode_[0])};

        return quad<sample_type>(
            triplet<sample_type>(traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                                 traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                                 traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3))),
            traits_.compute_reconstructed_sample(rb.v4, error_value4 * sign(rb.v4 - ra.v4)));
    }

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

    int32_t do_run_mode(const int32_t start_index)
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

    // codec parameters
    Traits traits_;
    uint32_t width_;
    uint8_t reset_threshold_{};

    // compression context
    std::array<context_regular_mode, 365> contexts_;
    std::array<context_run_mode, 2> context_run_mode_;
    int32_t run_index_{};
    pixel_type* previous_line_{};
    pixel_type* current_line_{};

    // quantization lookup table
    const int8_t* quantization_{};
    std::vector<int8_t> quantization_lut_;
};


} // namespace charls
