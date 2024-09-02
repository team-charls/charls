// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "make_scan_codec.hpp"

#include "default_traits.hpp"
#include "jpegls_preset_coding_parameters.hpp"
#include "lossless_traits.hpp"
#include "scan_decoder_impl.hpp"
#include "scan_encoder_impl.hpp"
#include "util.hpp"

namespace charls {

using std::make_unique;
using std::unique_ptr;

namespace {

/// <summary>
/// scan_codec_factory receives the actual frame info.
/// scan_codec expects 1 when encoding/decoding a single scan in interleave mode none.
/// </summary>
[[nodiscard]]
frame_info update_component_count(const frame_info& frame, const coding_parameters& parameters) noexcept
{
    return {frame.width, frame.height, frame.bits_per_sample,
            parameters.interleave_mode == interleave_mode::none ? 1 : frame.component_count};
}


template<typename ScanProcess, typename Traits>
[[nodiscard]]
unique_ptr<ScanProcess> make_codec(const frame_info& frame, const jpegls_pc_parameters& pc_parameters,
                                   const coding_parameters& parameters, const Traits& traits)
{
    if constexpr (std::is_same_v<ScanProcess, scan_encoder>)
    {
        return make_unique<scan_encoder_impl<Traits>>(update_component_count(frame, parameters), pc_parameters, parameters,
                                                      traits);
    }
    else
    {
        return make_unique<scan_decoder_impl<Traits>>(update_component_count(frame, parameters), pc_parameters, parameters,
                                                      traits);
    }
}

template<typename ScanProcess>
[[nodiscard]]
unique_ptr<ScanProcess> try_make_optimized_codec(const frame_info& frame, const jpegls_pc_parameters& pc_parameters,
                                                 const coding_parameters& parameters)
{
    if (parameters.interleave_mode == interleave_mode::sample && frame.component_count != 3 && frame.component_count != 4)
        return nullptr;

#ifndef DISABLE_SPECIALIZATIONS

    // optimized lossless versions common formats
    if (parameters.near_lossless == 0)
    {
        if (parameters.interleave_mode == interleave_mode::sample)
        {
            if (frame.component_count == 3 && frame.bits_per_sample == 8)
                return make_codec<ScanProcess>(frame, pc_parameters, parameters, lossless_traits<triplet<uint8_t>, 8>());
            if (frame.component_count == 4 && frame.bits_per_sample == 8)
                return make_codec<ScanProcess>(frame, pc_parameters, parameters, lossless_traits<quad<uint8_t>, 8>());
        }
        else
        {
            switch (frame.bits_per_sample)
            {
            case 8:
                return make_codec<ScanProcess>(frame, pc_parameters, parameters, lossless_traits<uint8_t, 8>());
            case 12:
                return make_codec<ScanProcess>(frame, pc_parameters, parameters, lossless_traits<uint16_t, 12>());
            case 16:
                return make_codec<ScanProcess>(frame, pc_parameters, parameters, lossless_traits<uint16_t, 16>());
            default:
                break;
            }
        }
    }

#endif

    const auto maximum_sample_value{calculate_maximum_sample_value(frame.bits_per_sample)};

    if (frame.bits_per_sample <= 8)
    {
        if (parameters.interleave_mode == interleave_mode::sample)
        {
            if (frame.component_count == 3)
            {
                return make_codec<ScanProcess>(
                    frame, pc_parameters, parameters,
                    default_traits<uint8_t, triplet<uint8_t>>(maximum_sample_value, parameters.near_lossless));
            }

            if (frame.component_count == 4)
            {
                return make_codec<ScanProcess>(
                    frame, pc_parameters, parameters,
                    default_traits<uint8_t, quad<uint8_t>>(maximum_sample_value, parameters.near_lossless));
            }
        }

        return make_codec<ScanProcess>(frame, pc_parameters, parameters,
                                       default_traits<uint8_t, uint8_t>(maximum_sample_value, parameters.near_lossless));
    }
    if (frame.bits_per_sample <= 16)
    {
        if (parameters.interleave_mode == interleave_mode::sample)
        {
            if (frame.component_count == 3)
            {
                return make_codec<ScanProcess>(
                    frame, pc_parameters, parameters,
                    default_traits<uint16_t, triplet<uint16_t>>(maximum_sample_value, parameters.near_lossless));
            }

            if (frame.component_count == 4)
            {
                return make_codec<ScanProcess>(
                    frame, pc_parameters, parameters,
                    default_traits<uint16_t, quad<uint16_t>>(maximum_sample_value, parameters.near_lossless));
            }
        }

        return make_codec<ScanProcess>(frame, pc_parameters, parameters,
                                       default_traits<uint16_t, uint16_t>(maximum_sample_value, parameters.near_lossless));
    }
    return nullptr;
}


} // namespace


template<typename ScanProcess>
unique_ptr<ScanProcess> make_scan_codec(const frame_info& frame, const jpegls_pc_parameters& pc_parameters,
                                        const coding_parameters& c_parameters)
{
    unique_ptr<ScanProcess> codec{try_make_optimized_codec<ScanProcess>(frame, pc_parameters, c_parameters)};

    if (!codec)
    {
        if (frame.bits_per_sample <= 8)
        {
            default_traits<uint8_t, uint8_t> traits(calculate_maximum_sample_value(frame.bits_per_sample),
                                                    c_parameters.near_lossless);
            traits.maximum_sample_value = pc_parameters.maximum_sample_value;
            codec = make_codec<ScanProcess, default_traits<uint8_t, uint8_t>>(frame, pc_parameters, c_parameters, traits);
        }
        else
        {
            default_traits<uint16_t, uint16_t> traits(calculate_maximum_sample_value(frame.bits_per_sample),
                                                      c_parameters.near_lossless);
            traits.maximum_sample_value = pc_parameters.maximum_sample_value;
            codec = make_codec<ScanProcess, default_traits<uint16_t, uint16_t>>(frame, pc_parameters, c_parameters, traits);
        }
    }

    return codec;
}


template std::unique_ptr<scan_decoder> make_scan_codec<scan_decoder>(const frame_info&, const jpegls_pc_parameters&,
                                                                     const coding_parameters&);
template std::unique_ptr<scan_encoder> make_scan_codec<scan_encoder>(const frame_info&, const jpegls_pc_parameters&,
                                                                     const coding_parameters&);

} // namespace charls
