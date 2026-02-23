// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "coding_parameters.hpp"
#include "jpegls_algorithm.hpp"
#include "sample_traits.hpp"
#include "scan_encoder_core.hpp"

namespace charls {

template<typename Traits>
class scan_encoder_impl final : public scan_encoder_core<sample_traits_t<Traits>>
{
    using base = scan_encoder_core<sample_traits_t<Traits>>;

public:
    using sample_type = typename Traits::sample_type;
    using pixel_type = typename Traits::pixel_type;

    scan_encoder_impl(const charls::frame_info& frame_info, const jpegls_pc_parameters& pc_parameters,
                      const coding_parameters& parameters, const Traits& traits) :
        base{frame_info, pc_parameters, parameters,
             copy_to_line_buffer<sample_type>::get_copy_function(parameters.interleave_mode, frame_info.component_count,
                                                                 frame_info.bits_per_sample, parameters.transformation),
             make_sample_traits(traits)},
        traits_{traits}
    {
        ASSERT(traits_.is_valid());
    }

    size_t encode_scan(const std::byte* source, const size_t stride, const span<std::byte> destination) override
    {
        this->initialize(destination);
        encode_lines(source, stride);
        this->end_scan();

        return this->get_length();
    }

private:
    [[nodiscard]]
    FORCE_INLINE int32_t quantize_gradient(const int32_t di) const noexcept
    {
        ASSERT(this->quantize_gradient_org(di, traits_.near_lossless) == *(this->quantization_ + di));
        return *(this->quantization_ + di);
    }

    // In ILV_SAMPLE mode, multiple components are handled in do_line
    // In ILV_LINE mode, a call to do_line is made for every component
    // In ILV_NONE mode, do_scan is called for each component
    void encode_lines(const std::byte* source, const size_t stride)
    {
        const uint32_t pixel_stride{this->width_ + 2U};
        const size_t component_count{this->parameters().interleave_mode == interleave_mode::line
                                         ? static_cast<size_t>(this->frame_info().component_count)
                                         : 1U};

        std::array<uint32_t, maximum_component_count_in_scan> run_index{};
        std::vector<pixel_type> line_buffer(component_count * pixel_stride * 2);

        for (uint32_t line{}; line < this->frame_info().height; ++line)
        {
            previous_line_ = line_buffer.data();
            current_line_ = line_buffer.data() + static_cast<size_t>(component_count) * pixel_stride;
            if ((line & 1) == 1)
            {
                std::swap(previous_line_, current_line_);
            }

            this->copy_source_to_line_buffer(source, current_line_ + 1, this->width_);
            source = source + stride;

            for (size_t component{}; component < component_count; ++component)
            {
                this->run_index_ = run_index[component];

                this->initialize_edge_pixels(previous_line_, current_line_, this->width_);

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

                run_index[component] = this->run_index_;
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

        while (index <= this->width_)
        {
            const int32_t ra{current_line_[index - 1]};
            const int32_t rc{rb};
            rb = rd;
            rd = previous_line_[index + 1];

            if (const int32_t qs{
                    compute_context_id(quantize_gradient(rd - rb), quantize_gradient(rb - rc), quantize_gradient(rc - ra))};
                qs != 0)
            {
                if constexpr (Traits::always_lossless)
                {
                    this->encode_regular_lossless(qs, current_line_[index], compute_predicted_value(ra, rb, rc));
                }
                else
                {
                    current_line_[index] =
                        this->encode_regular(qs, current_line_[index], compute_predicted_value(ra, rb, rc));
                }
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
        while (index <= this->width_)
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
                rx.v1 = this->encode_regular(qs1, current_line_[index].v1, compute_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = this->encode_regular(qs2, current_line_[index].v2, compute_predicted_value(ra.v2, rb.v2, rc.v2));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    /// <summary>Encodes a scan line of triplets in ILV_SAMPLE mode</summary>
    void encode_triplet_line()
    {
        size_t index{1};
        while (index <= this->width_)
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
                rx.v1 = this->encode_regular(qs1, current_line_[index].v1, compute_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = this->encode_regular(qs2, current_line_[index].v2, compute_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = this->encode_regular(qs3, current_line_[index].v3, compute_predicted_value(ra.v3, rb.v3, rc.v3));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    /// <summary>Encodes a scan line of quads in ILV_SAMPLE mode</summary>
    void encode_quad_line()
    {
        size_t index{1};
        while (index <= this->width_)
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
                rx.v1 = this->encode_regular(qs1, current_line_[index].v1, compute_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = this->encode_regular(qs2, current_line_[index].v2, compute_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = this->encode_regular(qs3, current_line_[index].v3, compute_predicted_value(ra.v3, rb.v3, rc.v3));
                rx.v4 = this->encode_regular(qs4, current_line_[index].v4, compute_predicted_value(ra.v4, rb.v4, rc.v4));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    [[nodiscard]]
    size_t encode_run_mode(const size_t start_index)
    {
        const size_t count_type_remain = this->width_ - (start_index - 1);
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

        this->encode_run_pixels(run_length, run_length == count_type_remain);

        if (run_length == count_type_remain)
            return run_length;

        type_cur_x[run_length] = encode_run_interruption_pixel(type_cur_x[run_length], ra, type_prev_x[run_length]);
        this->decrement_run_index();
        return run_length + 1;
    }

    [[nodiscard]]
    pair<sample_type> encode_run_interruption_pixel(const pair<sample_type> x, const pair<sample_type> ra,
                                                    const pair<sample_type> rb)
    {
        return {this->encode_run_interruption_component(x.v1, ra.v1, rb.v1),
                this->encode_run_interruption_component(x.v2, ra.v2, rb.v2)};
    }

    [[nodiscard]]
    triplet<sample_type> encode_run_interruption_pixel(const triplet<sample_type> x, const triplet<sample_type> ra,
                                                       const triplet<sample_type> rb)
    {
        return {this->encode_run_interruption_component(x.v1, ra.v1, rb.v1),
                this->encode_run_interruption_component(x.v2, ra.v2, rb.v2),
                this->encode_run_interruption_component(x.v3, ra.v3, rb.v3)};
    }

    [[nodiscard]]
    quad<sample_type> encode_run_interruption_pixel(const quad<sample_type> x, const quad<sample_type> ra,
                                                    const quad<sample_type> rb)
    {
        return {this->encode_run_interruption_component(x.v1, ra.v1, rb.v1),
                this->encode_run_interruption_component(x.v2, ra.v2, rb.v2),
                this->encode_run_interruption_component(x.v3, ra.v3, rb.v3),
                this->encode_run_interruption_component(x.v4, ra.v4, rb.v4)};
    }

    // For scalar pixel types, delegate to the base class scalar version
    using base::encode_run_interruption_pixel;

    Traits traits_;
    pixel_type* previous_line_{};
    pixel_type* current_line_{};
};

} // namespace charls
