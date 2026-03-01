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
    using base::decode_regular;
    using base::decode_run_interruption_component;
    using base::frame_info;
    using base::parameters_;
    using base::quantize_gradient;
    using base::run_index_;
    using base::width_;

    using pixel_type = typename Traits::pixel_type;
    using sample_type = typename Traits::sample_type;

public:

    scan_decoder_impl(const charls::frame_info& source_frame_info, const jpegls_pc_parameters& pc_parameters,
                      const coding_parameters& parameters, const Traits& traits) :
        base{source_frame_info, pc_parameters, parameters, make_sample_traits(traits)}, traits_{traits}
    {
        ASSERT(traits_.is_valid());

        base::copy_from_line_buffer_ = copy_from_line_buffer<sample_type>::get_copy_function(
            parameters.interleave_mode, source_frame_info.component_count, parameters.transformation);
    }

    [[nodiscard]]
    size_t decode_scan(const span<const std::byte> source, std::byte* destination, const size_t stride) override
    {
        const auto* scan_begin{to_address(source.begin())};

        base::initialize(source);

        // Process images without a restart interval, as 1 large restart interval.
        if (parameters_.restart_interval == 0)
        {
            parameters_.restart_interval = frame_info().height;
        }

        decode_lines(destination, stride);
        base::end_scan();

        return static_cast<size_t>(base::get_actual_position() - scan_begin);
    }

private:
    // In ILV_SAMPLE mode, multiple components are handled in do_line
    // In ILV_LINE mode, a call to do_line is made for every component
    // In ILV_NONE mode, do_scan is called for each component
    void decode_lines(std::byte* destination, const size_t stride)
    {
        const uint32_t pixel_stride{width_ + 2U};
        const size_t component_count{base::parameters().interleave_mode == interleave_mode::line
                                         ? static_cast<size_t>(frame_info().component_count)
                                         : 1U};

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

                    base::initialize_edge_pixels(previous_line_, current_line_, width_);

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

                base::copy_line_buffer_to_destination(current_line_ + 1 - (component_count * pixel_stride), destination,
                                                      width_);
                destination += stride;
            }

            if (line == frame_info().height)
                break;

            base::process_restart_marker();

            // After a restart marker it is required to reset the decoder.
            std::fill(run_index.begin(), run_index.end(), 0);
            std::fill(line_buffer.begin(), line_buffer.end(), pixel_type{});
            base::initialize_parameters(base::sample_traits_.range);
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

    /// <summary>Decodes a scan line of pairs in ILV_SAMPLE mode</summary>
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
        base::decrement_run_index();
        return end_index - start_index + 1;
    }

    [[nodiscard]]
    pair<sample_type> decode_run_interruption_pixel(const pair<sample_type> ra, const pair<sample_type> rb)
    {
        return {decode_run_interruption_component(ra.v1, rb.v1), decode_run_interruption_component(ra.v2, rb.v2)};
    }

    [[nodiscard]]
    triplet<sample_type> decode_run_interruption_pixel(const triplet<sample_type> ra, const triplet<sample_type> rb)
    {
        return {decode_run_interruption_component(ra.v1, rb.v1), decode_run_interruption_component(ra.v2, rb.v2),
                decode_run_interruption_component(ra.v3, rb.v3)};
    }

    [[nodiscard]]
    quad<sample_type> decode_run_interruption_pixel(const quad<sample_type> ra, const quad<sample_type> rb)
    {
        return {decode_run_interruption_component(ra.v1, rb.v1), decode_run_interruption_component(ra.v2, rb.v2),
                decode_run_interruption_component(ra.v3, rb.v3), decode_run_interruption_component(ra.v4, rb.v4)};
    }

    using base::decode_run_interruption_pixel;

    [[nodiscard]]
    size_t decode_run_pixels(pixel_type ra, pixel_type* start_pos, const size_t pixel_count)
    {
        size_t index{};
        while (base::read_bit())
        {
            const size_t count{std::min(size_t{1} << J[run_index_], pixel_count - index)};
            index += count;
            ASSERT(index <= pixel_count);

            if (count == (size_t{1} << J[run_index_]))
            {
                base::increment_run_index();
            }

            if (index == pixel_count)
                break;
        }

        if (index != pixel_count)
        {
            // Incomplete run.
            index += (J[run_index_] > 0) ? base::read_value(J[run_index_]) : 0;
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
