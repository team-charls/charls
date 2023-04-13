// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "default_traits.h"
#include "jls_codec_factory.h"
#include "jpegls_preset_coding_parameters.h"
#include "lossless_traits.h"
#include "scan.h"
#include "util.h"

#include <vector>

namespace charls {

using std::make_unique;
using std::unique_ptr;
using std::vector;

namespace {

// See JPEG-LS standard ISO/IEC 14495-1, A.3.3, golomb_code Segment A.4
int8_t quantize_gradient_org(const jpegls_pc_parameters& preset, const int32_t di) noexcept
{
    constexpr int32_t near_lossless{};

    if (di <= -preset.threshold3)
        return -4;
    if (di <= -preset.threshold2)
        return -3;
    if (di <= -preset.threshold1)
        return -2;
    if (di < -near_lossless)
        return -1;
    if (di <= near_lossless)
        return 0;
    if (di < preset.threshold1)
        return 1;
    if (di < preset.threshold2)
        return 2;
    if (di < preset.threshold3)
        return 3;

    return 4;
}

vector<int8_t> create_quantize_lut_lossless(const int32_t bit_count)
{
    const jpegls_pc_parameters preset{compute_default(calculate_maximum_sample_value(bit_count), 0)};
    const int32_t range{preset.maximum_sample_value + 1};

    vector<int8_t> lut(static_cast<size_t>(range) * 2);
    for (size_t i{}; i != lut.size(); ++i)
    {
        lut[i] = quantize_gradient_org(preset, static_cast<int32_t>(i) - range);
    }

    return lut;
}

template<typename Strategy, typename Traits>
unique_ptr<Strategy> make_codec(const Traits& traits, const frame_info& frame_info, const coding_parameters& parameters)
{
    if constexpr (std::is_same_v<Strategy, encoder_strategy>)
    {
        return make_unique<scan_encoder_implementation<Traits>>(traits, frame_info, parameters);
    }
    else
    {
        return make_unique<scan_decoder_implementation<Traits>>(traits, frame_info, parameters);
    }
}

} // namespace


// Lookup tables: sample differences to bin indexes.
// ReSharper disable CppTemplateArgumentsCanBeDeduced
// NOLINTNEXTLINE(clang-diagnostic-global-constructors)
const vector<int8_t> quantization_lut_lossless_8{create_quantize_lut_lossless(8)};

// NOLINTNEXTLINE(clang-diagnostic-global-constructors)
const vector<int8_t> quantization_lut_lossless_10{create_quantize_lut_lossless(10)};

// NOLINTNEXTLINE(clang-diagnostic-global-constructors)
const vector<int8_t> quantization_lut_lossless_12{create_quantize_lut_lossless(12)};

// NOLINTNEXTLINE(clang-diagnostic-global-constructors)
const vector<int8_t> quantization_lut_lossless_16{create_quantize_lut_lossless(16)};
// ReSharper restore CppTemplateArgumentsCanBeDeduced


template<typename Strategy>
unique_ptr<Strategy> jls_codec_factory<Strategy>::create_codec(const frame_info& frame, const coding_parameters& parameters,
                                                               const jpegls_pc_parameters& preset_coding_parameters)
{
    unique_ptr<Strategy> codec;

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
            codec = make_codec<Strategy, default_traits<uint8_t, uint8_t>>(traits, frame, parameters);
        }
        else
        {
            default_traits<uint16_t, uint16_t> traits(calculate_maximum_sample_value(frame.bits_per_sample),
                                                      parameters.near_lossless, preset_coding_parameters.reset_value);
            traits.maximum_sample_value = preset_coding_parameters.maximum_sample_value;
            codec = make_codec<Strategy, default_traits<uint16_t, uint16_t>>(traits, frame, parameters);
        }
    }

    codec->set_presets(preset_coding_parameters);
    return codec;
}

template<typename Strategy>
unique_ptr<Strategy> jls_codec_factory<Strategy>::try_create_optimized_codec(const frame_info& frame,
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
                return make_codec<Strategy>(lossless_traits<triplet<uint8_t>, 8>(), frame, parameters);
            if (frame.component_count == 4 && frame.bits_per_sample == 8)
                return make_codec<Strategy>(lossless_traits<quad<uint8_t>, 8>(), frame, parameters);
        }
        else
        {
            switch (frame.bits_per_sample)
            {
            case 8:
                return make_codec<Strategy>(lossless_traits<uint8_t, 8>(), frame, parameters);
            case 12:
                return make_codec<Strategy>(lossless_traits<uint16_t, 12>(), frame, parameters);
            case 16:
                return make_codec<Strategy>(lossless_traits<uint16_t, 16>(), frame, parameters);
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
                return make_codec<Strategy>(default_traits<uint8_t, triplet<uint8_t>>(maxval, parameters.near_lossless),
                                            frame, parameters);
            }

            if (frame.component_count == 4)
            {
                return make_codec<Strategy>(default_traits<uint8_t, quad<uint8_t>>(maxval, parameters.near_lossless), frame,
                                            parameters);
            }
        }

        return make_codec<Strategy>(default_traits<uint8_t, uint8_t>(maxval, parameters.near_lossless), frame, parameters);
    }
    if (frame.bits_per_sample <= 16)
    {
        if (parameters.interleave_mode == interleave_mode::sample)
        {
            if (frame.component_count == 3)
            {
                return make_codec<Strategy>(default_traits<uint16_t, triplet<uint16_t>>(maxval, parameters.near_lossless),
                                            frame, parameters);
            }

            if (frame.component_count == 4)
            {
                return make_codec<Strategy>(default_traits<uint16_t, quad<uint16_t>>(maxval, parameters.near_lossless),
                                            frame, parameters);
            }
        }

        return make_codec<Strategy>(default_traits<uint16_t, uint16_t>(maxval, parameters.near_lossless), frame, parameters);
    }
    return nullptr;
}


template class jls_codec_factory<decoder_strategy>;
template class jls_codec_factory<encoder_strategy>;

} // namespace charls
