// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "color_transform.hpp"
#include "sample_traits.hpp"
#include "scan_decoder_core.hpp"

namespace charls {

template<typename Traits>
class scan_decoder_impl final : public scan_decoder_core<sample_traits_t<Traits>>
{
    using base = scan_decoder_core<sample_traits_t<Traits>>;

public:
    using pixel_type = typename Traits::pixel_type;
    using sample_type = typename Traits::sample_type;

    scan_decoder_impl(const charls::frame_info& frame_info, const jpegls_pc_parameters& pc_parameters,
                      const coding_parameters& parameters, const Traits& traits) :
        base{frame_info, pc_parameters, parameters, make_sample_traits(traits)}, traits_{traits}
    {
        ASSERT(traits_.is_valid());

        this->copy_from_line_buffer_ = copy_from_line_buffer<sample_type>::get_copy_function(
            parameters.interleave_mode, frame_info.component_count, parameters.transformation);
    }

    [[nodiscard]]
    size_t decode_scan(const span<const std::byte> source, std::byte* destination, const size_t stride) override
    {
        const auto* scan_begin{to_address(source.begin())};

        this->initialize(source);

        // Process images without a restart interval, as 1 large restart interval.
        if (this->parameters_.restart_interval == 0)
        {
            this->parameters_.restart_interval = this->frame_info().height;
        }

        decode_lines(destination, stride);
        this->end_scan();

        return static_cast<size_t>(this->get_actual_position() - scan_begin);
    }

private:
    // In ILV_SAMPLE mode, multiple components are handled in do_line
    // In ILV_LINE mode, a call to do_line is made for every component
    // In ILV_NONE mode, do_scan is called for each component
    void decode_lines(std::byte* destination, const size_t stride)
    {
        const uint32_t pixel_stride{this->width_ + 2U};
        const size_t component_count{this->parameters().interleave_mode == interleave_mode::line
                                         ? static_cast<size_t>(this->frame_info().component_count)
                                         : 1U};

        std::array<uint32_t, maximum_component_count_in_scan> run_index{};
        std::vector<pixel_type> line_buffer(component_count * pixel_stride * 2);

        for (uint32_t line{};;)
        {
            const uint32_t lines_in_interval{std::min(this->frame_info().height - line, this->parameters_.restart_interval)};

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
                    this->run_index_ = run_index[component];

                    this->initialize_edge_pixels(previous_line_, current_line_, this->width_);

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

                    run_index[component] = this->run_index_;
                    current_line_ += pixel_stride;
                    previous_line_ += pixel_stride;
                }

                this->copy_line_buffer_to_destination(current_line_ + 1 - (component_count * pixel_stride), destination,
                                                      this->width_);
                destination += stride;
            }

            if (line == this->frame_info().height)
                break;

            this->process_restart_marker();

            // After a restart marker it is required to reset the decoder.
            std::fill(run_index.begin(), run_index.end(), 0);
            std::fill(line_buffer.begin(), line_buffer.end(), pixel_type{});
            this->initialize_parameters(this->sample_traits_.range);
        }
    }

    /// <summary>Decodes a scan line of samples</summary>
    FORCE_INLINE void decode_sample_line()
    {
        size_t index{1};
        int32_t rb{*previous_line_};       // initial start value is rc, will be copied and overwritten in loop.
        int32_t rd{previous_line_[index]}; // initial start value is rb, will be copied and overwritten in loop.

        while (index <= this->width_)
        {
            const int32_t ra{current_line_[index - 1]};
            const int32_t rc{rb};
            rb = rd;
            rd = previous_line_[index + 1];

            if (const int32_t qs{
                    compute_context_id(this->quantize_gradient(rd - rb), this->quantize_gradient(rb - rc), this->quantize_gradient(rc - ra))};
                qs != 0)
            {
                current_line_[index] = this->decode_regular(qs, compute_predicted_value(ra, rb, rc));
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

    /// <summary>Decodes a scan line of pairs in ILV_SAMPLE mode</summary>
    void decode_pair_line()
    {
        size_t index{1};
        while (index <= this->width_)
        {
            const pair<sample_type> ra{current_line_[index - 1]};
            const pair<sample_type> rc{previous_line_[index - 1]};
            const pair<sample_type> rb{previous_line_[index]};
            const pair<sample_type> rd{previous_line_[index + 1]};

            const int32_t qs1{compute_context_id(this->quantize_gradient(rd.v1 - rb.v1), this->quantize_gradient(rb.v1 - rc.v1),
                                                 this->quantize_gradient(rc.v1 - ra.v1))};
            const int32_t qs2{compute_context_id(this->quantize_gradient(rd.v2 - rb.v2), this->quantize_gradient(rb.v2 - rc.v2),
                                                 this->quantize_gradient(rc.v2 - ra.v2))};

            if (qs1 == 0 && qs2 == 0)
            {
                index += decode_run_mode(index);
            }
            else
            {
                pair<sample_type> rx;
                rx.v1 = this->decode_regular(qs1, compute_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = this->decode_regular(qs2, compute_predicted_value(ra.v2, rb.v2, rc.v2));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    /// <summary>Decodes a scan line of triplets in ILV_SAMPLE mode</summary>
    void decode_triplet_line()
    {
        size_t index{1};
        while (index <= this->width_)
        {
            const triplet<sample_type> ra{current_line_[index - 1]};
            const triplet<sample_type> rc{previous_line_[index - 1]};
            const triplet<sample_type> rb{previous_line_[index]};
            const triplet<sample_type> rd{previous_line_[index + 1]};

            const int32_t qs1{compute_context_id(this->quantize_gradient(rd.v1 - rb.v1), this->quantize_gradient(rb.v1 - rc.v1),
                                                 this->quantize_gradient(rc.v1 - ra.v1))};
            const int32_t qs2{compute_context_id(this->quantize_gradient(rd.v2 - rb.v2), this->quantize_gradient(rb.v2 - rc.v2),
                                                 this->quantize_gradient(rc.v2 - ra.v2))};

            if (const int32_t qs3{compute_context_id(this->quantize_gradient(rd.v3 - rb.v3), this->quantize_gradient(rb.v3 - rc.v3),
                                                     this->quantize_gradient(rc.v3 - ra.v3))};
                qs1 == 0 && qs2 == 0 && qs3 == 0)
            {
                index += decode_run_mode(index);
            }
            else
            {
                triplet<sample_type> rx;
                rx.v1 = this->decode_regular(qs1, compute_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = this->decode_regular(qs2, compute_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = this->decode_regular(qs3, compute_predicted_value(ra.v3, rb.v3, rc.v3));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    /// <summary>Decodes a scan line of quads in ILV_SAMPLE mode</summary>
    void decode_quad_line()
    {
        size_t index{1};
        while (index <= this->width_)
        {
            const quad<sample_type> ra{current_line_[index - 1]};
            const quad<sample_type> rc{previous_line_[index - 1]};
            const quad<sample_type> rb{previous_line_[index]};
            const quad<sample_type> rd{previous_line_[index + 1]};

            const int32_t qs1{compute_context_id(this->quantize_gradient(rd.v1 - rb.v1), this->quantize_gradient(rb.v1 - rc.v1),
                                                 this->quantize_gradient(rc.v1 - ra.v1))};
            const int32_t qs2{compute_context_id(this->quantize_gradient(rd.v2 - rb.v2), this->quantize_gradient(rb.v2 - rc.v2),
                                                 this->quantize_gradient(rc.v2 - ra.v2))};
            const int32_t qs3{compute_context_id(this->quantize_gradient(rd.v3 - rb.v3), this->quantize_gradient(rb.v3 - rc.v3),
                                                 this->quantize_gradient(rc.v3 - ra.v3))};

            if (const int32_t qs4{compute_context_id(this->quantize_gradient(rd.v4 - rb.v4), this->quantize_gradient(rb.v4 - rc.v4),
                                                     this->quantize_gradient(rc.v4 - ra.v4))};
                qs1 == 0 && qs2 == 0 && qs3 == 0 && qs4 == 0)
            {
                index += decode_run_mode(index);
            }
            else
            {
                quad<sample_type> rx;
                rx.v1 = this->decode_regular(qs1, compute_predicted_value(ra.v1, rb.v1, rc.v1));
                rx.v2 = this->decode_regular(qs2, compute_predicted_value(ra.v2, rb.v2, rc.v2));
                rx.v3 = this->decode_regular(qs3, compute_predicted_value(ra.v3, rb.v3, rc.v3));
                rx.v4 = this->decode_regular(qs4, compute_predicted_value(ra.v4, rb.v4, rc.v4));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    [[nodiscard]]
    size_t decode_run_mode(const size_t start_index)
    {
        const pixel_type ra{current_line_[start_index - 1]};

        const size_t run_length{decode_run_pixels(ra, current_line_ + start_index, this->width_ - (start_index - 1))};
        const auto end_index{static_cast<uint32_t>(start_index + run_length)};

        if (end_index - 1 == this->width_)
            return end_index - start_index;

        // Run interruption
        const pixel_type rb{previous_line_[end_index]};
        current_line_[end_index] = decode_run_interruption_pixel(ra, rb);
        this->decrement_run_index();
        return end_index - start_index + 1;
    }

    [[nodiscard]]
    pair<sample_type> decode_run_interruption_pixel(const pair<sample_type> ra, const pair<sample_type> rb)
    {
        return {this->decode_run_interruption_component(ra.v1, rb.v1),
                this->decode_run_interruption_component(ra.v2, rb.v2)};
    }

    [[nodiscard]]
    triplet<sample_type> decode_run_interruption_pixel(const triplet<sample_type> ra, const triplet<sample_type> rb)
    {
        return {this->decode_run_interruption_component(ra.v1, rb.v1),
                this->decode_run_interruption_component(ra.v2, rb.v2),
                this->decode_run_interruption_component(ra.v3, rb.v3)};
    }

    [[nodiscard]]
    quad<sample_type> decode_run_interruption_pixel(const quad<sample_type> ra, const quad<sample_type> rb)
    {
        return {this->decode_run_interruption_component(ra.v1, rb.v1),
                this->decode_run_interruption_component(ra.v2, rb.v2),
                this->decode_run_interruption_component(ra.v3, rb.v3),
                this->decode_run_interruption_component(ra.v4, rb.v4)};
    }

    using base::decode_run_interruption_pixel;

    [[nodiscard]]
    size_t decode_run_pixels(pixel_type ra, pixel_type* start_pos, const size_t pixel_count)
    {
        size_t index{};
        while (this->read_bit())
        {
            const size_t count{std::min(size_t{1} << J[this->run_index_], pixel_count - index)};
            index += count;
            ASSERT(index <= pixel_count);

            if (count == (size_t{1} << J[this->run_index_]))
            {
                this->increment_run_index();
            }

            if (index == pixel_count)
                break;
        }

        if (index != pixel_count)
        {
            // Incomplete run.
            index += (J[this->run_index_] > 0) ? this->read_value(J[this->run_index_]) : 0;
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
