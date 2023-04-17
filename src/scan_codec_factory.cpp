// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "scan_codec_factory.h"

#include "default_traits.h"
#include "jpegls_preset_coding_parameters.h"
#include "lossless_traits.h"
#include "scan_decoder_impl.h"
#include "scan_encoder_impl.h"
#include "util.h"

namespace charls {

using std::make_unique;
using std::unique_ptr;

namespace {

/// <summary>
/// scan_codec_factory receives the actual frame info. scan_codec expects 1 when encoding/decoding a single scan in interleave mode none.
/// </summary>
[[nodiscard]] frame_info update_component_count(const frame_info& frame, const coding_parameters& parameters) noexcept
{
    return {frame.width, frame.height, frame.bits_per_sample,
            parameters.interleave_mode == interleave_mode::none ? 1 : frame.component_count};
}


template<typename ScanProcess, typename Traits>
unique_ptr<ScanProcess> make_codec(const Traits& traits, const frame_info& frame, const coding_parameters& parameters)
{
    if constexpr (std::is_same_v<ScanProcess, scan_encoder>)
    {
        return make_unique<scan_encoder_impl<Traits>>(traits, update_component_count(frame, parameters), parameters);
    }
    else
    {
        return make_unique<scan_decoder_impl<Traits>>(traits, update_component_count(frame, parameters), parameters);
    }
}

} // namespace


template<typename ScanProcess>
unique_ptr<ScanProcess> scan_codec_factory<ScanProcess>::create_codec(const frame_info& frame,
                                                                      const coding_parameters& parameters,
                                                                      const jpegls_pc_parameters& preset_coding_parameters)
{
    unique_ptr<ScanProcess> codec;

    if (preset_coding_parameters.reset_value == default_reset_value)
    {
        codec = try_create_optimized_codec(frame, parameters);
    }

    if (!codec)
    {
        if (frame.bits_per_sample <= 8)
        {
            default_traits<uint8_t, uint8_t> traits(calculate_maximum_sample_value(frame.bits_per_sample),
                                                    parameters.near_lossless, preset_coding_parameters.reset_value);
            traits.maximum_sample_value = preset_coding_parameters.maximum_sample_value;
            codec = make_codec<ScanProcess, default_traits<uint8_t, uint8_t>>(traits, frame, parameters);
        }
        else
        {
            default_traits<uint16_t, uint16_t> traits(calculate_maximum_sample_value(frame.bits_per_sample),
                                                      parameters.near_lossless, preset_coding_parameters.reset_value);
            traits.maximum_sample_value = preset_coding_parameters.maximum_sample_value;
            codec = make_codec<ScanProcess, default_traits<uint16_t, uint16_t>>(traits, frame, parameters);
        }
    }

    codec->set_presets(preset_coding_parameters);
    return codec;
}

template<typename ScanProcess>
unique_ptr<ScanProcess> scan_codec_factory<ScanProcess>::try_create_optimized_codec(const frame_info& frame,
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
                return make_codec<ScanProcess>(lossless_traits<triplet<uint8_t>, 8>(), frame, parameters);
            if (frame.component_count == 4 && frame.bits_per_sample == 8)
                return make_codec<ScanProcess>(lossless_traits<quad<uint8_t>, 8>(), frame, parameters);
        }
        else
        {
            switch (frame.bits_per_sample)
            {
            case 8:
                return make_codec<ScanProcess>(lossless_traits<uint8_t, 8>(), frame, parameters);
            case 12:
                return make_codec<ScanProcess>(lossless_traits<uint16_t, 12>(), frame, parameters);
            case 16:
                return make_codec<ScanProcess>(lossless_traits<uint16_t, 16>(), frame, parameters);
            default:
                break;
            }
        }
    }

#endif

    const auto maxval{calculate_maximum_sample_value(frame.bits_per_sample)};

    if (frame.bits_per_sample <= 8)
    {
        if (parameters.interleave_mode == interleave_mode::sample)
        {
            if (frame.component_count == 3)
            {
                return make_codec<ScanProcess>(default_traits<uint8_t, triplet<uint8_t>>(maxval, parameters.near_lossless),
                                               frame, parameters);
            }

            if (frame.component_count == 4)
            {
                return make_codec<ScanProcess>(default_traits<uint8_t, quad<uint8_t>>(maxval, parameters.near_lossless),
                                               frame, parameters);
            }
        }

        return make_codec<ScanProcess>(default_traits<uint8_t, uint8_t>(maxval, parameters.near_lossless), frame,
                                       parameters);
    }
    if (frame.bits_per_sample <= 16)
    {
        if (parameters.interleave_mode == interleave_mode::sample)
        {
            if (frame.component_count == 3)
            {
                return make_codec<ScanProcess>(default_traits<uint16_t, triplet<uint16_t>>(maxval, parameters.near_lossless),
                                               frame, parameters);
            }

            if (frame.component_count == 4)
            {
                return make_codec<ScanProcess>(default_traits<uint16_t, quad<uint16_t>>(maxval, parameters.near_lossless),
                                               frame, parameters);
            }
        }

        return make_codec<ScanProcess>(default_traits<uint16_t, uint16_t>(maxval, parameters.near_lossless), frame,
                                       parameters);
    }
    return nullptr;
}


template class scan_codec_factory<scan_decoder>;
template class scan_codec_factory<scan_encoder>;

} // namespace charls
