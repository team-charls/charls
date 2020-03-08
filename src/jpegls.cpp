// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "decoder_strategy.h"
#include "default_traits.h"
#include "encoder_strategy.h"
#include "jls_codec_factory.h"
#include "jpeg_stream_reader.h"
#include "jpegls_preset_coding_parameters.h"
#include "lookup_table.h"
#include "lossless_traits.h"
#include "util.h"

#include <array>
#include <vector>

// As defined in the JPEG-LS standard

// used to determine how large runs should be encoded at a time.
const std::array<int, 32> J = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 11, 12, 13, 14, 15};

#include "scan.h"

using std::array;
using std::make_unique;
using std::unique_ptr;
using std::vector;
using namespace charls;

namespace {

signed char QuantizeGradientOrg(const jpegls_pc_parameters& preset, const int32_t near_lossless, const int32_t Di) noexcept
{
    if (Di <= -preset.threshold3) return -4;
    if (Di <= -preset.threshold2) return -3;
    if (Di <= -preset.threshold1) return -2;
    if (Di < -near_lossless) return -1;
    if (Di <= near_lossless) return 0;
    if (Di < preset.threshold1) return 1;
    if (Di < preset.threshold2) return 2;
    if (Di < preset.threshold3) return 3;

    return 4;
}

vector<signed char> CreateQLutLossless(const int32_t bitCount)
{
    const jpegls_pc_parameters preset{compute_default((1U << static_cast<uint32_t>(bitCount)) - 1, 0)};
    const int32_t range = preset.maximum_sample_value + 1;

    vector<signed char> lut(static_cast<size_t>(range) * 2);

    for (int32_t diff = -range; diff < range; diff++)
    {
        lut[static_cast<size_t>(range) + diff] = QuantizeGradientOrg(preset, 0, diff);
    }
    return lut;
}

template<typename Strategy, typename Traits>
unique_ptr<Strategy> create_codec(const Traits& traits, const frame_info& frame_info, const coding_parameters& parameters)
{
    return make_unique<charls::JlsCodec<Traits, Strategy>>(traits, frame_info, parameters);
}

} // namespace


namespace charls {

// Lookup tables to replace code with lookup tables.
// To avoid threading issues, all tables are created when the program is loaded.

// Lookup table: decode symbols that are smaller or equal to 8 bit (16 tables for each value of k)
array<CTable, 16> decodingTables = {InitTable(0), InitTable(1), InitTable(2), InitTable(3), // NOLINT(clang-diagnostic-global-constructors)
                                    InitTable(4), InitTable(5), InitTable(6), InitTable(7),
                                    InitTable(8), InitTable(9), InitTable(10), InitTable(11),
                                    InitTable(12), InitTable(13), InitTable(14), InitTable(15)};

// Lookup tables: sample differences to bin indexes.
vector<signed char> rgquant8Ll = CreateQLutLossless(8);   // NOLINT(clang-diagnostic-global-constructors)
vector<signed char> rgquant10Ll = CreateQLutLossless(10); // NOLINT(clang-diagnostic-global-constructors)
vector<signed char> rgquant12Ll = CreateQLutLossless(12); // NOLINT(clang-diagnostic-global-constructors)
vector<signed char> rgquant16Ll = CreateQLutLossless(16); // NOLINT(clang-diagnostic-global-constructors)


template<typename Strategy>
unique_ptr<Strategy> JlsCodecFactory<Strategy>::CreateCodec(const frame_info& frame, const coding_parameters& parameters, const jpegls_pc_parameters& preset_coding_parameters)
{
    unique_ptr<Strategy> codec;

    if (preset_coding_parameters.reset_value == 0 || preset_coding_parameters.reset_value == DefaultResetValue)
    {
        codec = CreateOptimizedCodec(frame, parameters);
    }

    if (!codec)
    {
        if (frame.bits_per_sample <= 8)
        {
            DefaultTraits<uint8_t, uint8_t> traits(calculate_maximum_sample_value(frame.bits_per_sample), parameters.near_lossless, preset_coding_parameters.reset_value);
            traits.MAXVAL = preset_coding_parameters.maximum_sample_value;
            codec = make_unique<JlsCodec<DefaultTraits<uint8_t, uint8_t>, Strategy>>(traits, frame, parameters);
        }
        else
        {
            DefaultTraits<uint16_t, uint16_t> traits(calculate_maximum_sample_value(frame.bits_per_sample), parameters.near_lossless, preset_coding_parameters.reset_value);
            traits.MAXVAL = preset_coding_parameters.maximum_sample_value;
            codec = make_unique<JlsCodec<DefaultTraits<uint16_t, uint16_t>, Strategy>>(traits, frame, parameters);
        }
    }

    codec->SetPresets(preset_coding_parameters);
    return codec;
}

template<typename Strategy>
unique_ptr<Strategy> JlsCodecFactory<Strategy>::CreateOptimizedCodec(const frame_info& frame, const coding_parameters& parameters)
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
                return create_codec<Strategy>(LosslessTraits<Triplet<uint8_t>, 8>(), frame, parameters);
            if (frame.component_count == 4 && frame.bits_per_sample == 8)
                return create_codec<Strategy>(LosslessTraits<Quad<uint8_t>, 8>(), frame, parameters);
        }
        else
        {
            switch (frame.bits_per_sample)
            {
            case 8:
                return create_codec<Strategy>(LosslessTraits<uint8_t, 8>(), frame, parameters);
            case 12:
                return create_codec<Strategy>(LosslessTraits<uint16_t, 12>(), frame, parameters);
            case 16:
                return create_codec<Strategy>(LosslessTraits<uint16_t, 16>(), frame, parameters);
            default:
                break;
            }
        }
    }

#endif

    const int maxval = calculate_maximum_sample_value(frame.bits_per_sample);

    if (frame.bits_per_sample <= 8)
    {
        if (parameters.interleave_mode == interleave_mode::sample)
        {
            if (frame.component_count == 3)
                return create_codec<Strategy>(DefaultTraits<uint8_t, Triplet<uint8_t>>(maxval, parameters.near_lossless), frame, parameters);
            if (frame.component_count == 4)
                return create_codec<Strategy>(DefaultTraits<uint8_t, Quad<uint8_t>>(maxval, parameters.near_lossless), frame, parameters);
        }

        return create_codec<Strategy>(DefaultTraits<uint8_t, uint8_t>(maxval, parameters.near_lossless), frame, parameters);
    }
    if (frame.bits_per_sample <= 16)
    {
        if (parameters.interleave_mode == interleave_mode::sample)
        {
            if (frame.component_count == 3)
                return create_codec<Strategy>(DefaultTraits<uint16_t, Triplet<uint16_t>>(maxval, parameters.near_lossless), frame, parameters);
            if (frame.component_count == 4)
                return create_codec<Strategy>(DefaultTraits<uint16_t, Quad<uint16_t>>(maxval, parameters.near_lossless), frame, parameters);
        }

        return create_codec<Strategy>(DefaultTraits<uint16_t, uint16_t>(maxval, parameters.near_lossless), frame, parameters);
    }
    return nullptr;
}


template class JlsCodecFactory<DecoderStrategy>;
template class JlsCodecFactory<EncoderStrategy>;

} // namespace charls
