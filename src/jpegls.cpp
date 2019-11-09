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

#include <vector>

// As defined in the JPEG-LS standard

// used to determine how large runs should be encoded at a time.
const int J[32] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 11, 12, 13, 14, 15};

#include "scan.h"

using std::make_unique;
using std::unique_ptr;
using std::vector;
using namespace charls;

namespace {

signed char QuantizeGradientOrg(const jpegls_pc_parameters& preset, int32_t near_lossless, int32_t Di) noexcept
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

vector<signed char> CreateQLutLossless(int32_t bitCount)
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
unique_ptr<Strategy> create_codec(const Traits& traits, const JlsParameters& params)
{
    return make_unique<charls::JlsCodec<Traits, Strategy>>(traits, params);
}

} // namespace


namespace charls {

// Lookup tables to replace code with lookup tables.
// To avoid threading issues, all tables are created when the program is loaded.

// Lookup table: decode symbols that are smaller or equal to 8 bit (16 tables for each value of k)
CTable decodingTables[16] = {InitTable(0), InitTable(1), InitTable(2), InitTable(3),
                             InitTable(4), InitTable(5), InitTable(6), InitTable(7),
                             InitTable(8), InitTable(9), InitTable(10), InitTable(11),
                             InitTable(12), InitTable(13), InitTable(14), InitTable(15)};

// Lookup tables: sample differences to bin indexes.
vector<signed char> rgquant8Ll = CreateQLutLossless(8);
vector<signed char> rgquant10Ll = CreateQLutLossless(10);
vector<signed char> rgquant12Ll = CreateQLutLossless(12);
vector<signed char> rgquant16Ll = CreateQLutLossless(16);


template<typename Strategy>
unique_ptr<Strategy> JlsCodecFactory<Strategy>::CreateCodec(const JlsParameters& params, const jpegls_pc_parameters& preset_coding_parameters)
{
    unique_ptr<Strategy> codec;

    if (preset_coding_parameters.reset_value == 0 || preset_coding_parameters.reset_value == DefaultResetValue)
    {
        codec = CreateOptimizedCodec(params);
    }

    if (!codec)
    {
        if (params.bitsPerSample <= 8)
        {
            DefaultTraits<uint8_t, uint8_t> traits((1 << params.bitsPerSample) - 1, params.allowedLossyError, preset_coding_parameters.reset_value);
            traits.MAXVAL = preset_coding_parameters.maximum_sample_value;
            codec = make_unique<JlsCodec<DefaultTraits<uint8_t, uint8_t>, Strategy>>(traits, params);
        }
        else
        {
            DefaultTraits<uint16_t, uint16_t> traits((1 << params.bitsPerSample) - 1, params.allowedLossyError, preset_coding_parameters.reset_value);
            traits.MAXVAL = preset_coding_parameters.maximum_sample_value;
            codec = make_unique<JlsCodec<DefaultTraits<uint16_t, uint16_t>, Strategy>>(traits, params);
        }
    }

    codec->SetPresets(preset_coding_parameters);
    return codec;
}

template<typename Strategy>
unique_ptr<Strategy> JlsCodecFactory<Strategy>::CreateOptimizedCodec(const JlsParameters& params)
{
    if (params.interleaveMode == interleave_mode::sample && params.components != 3 && params.components != 4)
        return nullptr;

#ifndef DISABLE_SPECIALIZATIONS

    // optimized lossless versions common formats
    if (params.allowedLossyError == 0)
    {
        if (params.interleaveMode == interleave_mode::sample)
        {
            if (params.components == 3 && params.bitsPerSample == 8)
                return create_codec<Strategy>(LosslessTraits<Triplet<uint8_t>, 8>(), params);
            if (params.components == 4 && params.bitsPerSample == 8)
                return create_codec<Strategy>(LosslessTraits<Quad<uint8_t>, 8>(), params);
        }
        else
        {
            switch (params.bitsPerSample)
            {
            case 8:
                return create_codec<Strategy>(LosslessTraits<uint8_t, 8>(), params);
            case 12:
                return create_codec<Strategy>(LosslessTraits<uint16_t, 12>(), params);
            case 16:
                return create_codec<Strategy>(LosslessTraits<uint16_t, 16>(), params);
            default:
                break;
            }
        }
    }

#endif

    const int maxval = (1u << static_cast<unsigned int>(params.bitsPerSample)) - 1;

    if (params.bitsPerSample <= 8)
    {
        if (params.interleaveMode == interleave_mode::sample)
        {
            if (params.components == 3)
                return create_codec<Strategy>(DefaultTraits<uint8_t, Triplet<uint8_t>>(maxval, params.allowedLossyError), params);
            if (params.components == 4)
                return create_codec<Strategy>(DefaultTraits<uint8_t, Quad<uint8_t>>(maxval, params.allowedLossyError), params);
        }

        return create_codec<Strategy>(DefaultTraits<uint8_t, uint8_t>((1u << params.bitsPerSample) - 1, params.allowedLossyError), params);
    }
    if (params.bitsPerSample <= 16)
    {
        if (params.interleaveMode == interleave_mode::sample)
        {
            if (params.components == 3)
                return create_codec<Strategy>(DefaultTraits<uint16_t, Triplet<uint16_t>>(maxval, params.allowedLossyError), params);
            if (params.components == 4)
                return create_codec<Strategy>(DefaultTraits<uint16_t, Quad<uint16_t>>(maxval, params.allowedLossyError), params);
        }

        return create_codec<Strategy>(DefaultTraits<uint16_t, uint16_t>(maxval, params.allowedLossyError), params);
    }
    return nullptr;
}


template class JlsCodecFactory<DecoderStrategy>;
template class JlsCodecFactory<EncoderStrategy>;

} // namespace charls
