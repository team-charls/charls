// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "coding_parameters.h"
#include "color_transform.h"
#include "context.h"
#include "context_run_mode.h"
#include "lookup_table.h"
#include "process_line.h"

#include <array>
#include <sstream>

// This file contains the code for handling a "scan". Usually an image is encoded as a single scan.
// Note: the functions in this header could be moved into jpegls.cpp as they are only used in that file.

namespace charls {

class decoder_strategy;
class encoder_strategy;

extern std::array<golomb_code_table, 16> decodingTables;
extern std::vector<int8_t> quantization_lut_lossless_8;
extern std::vector<int8_t> quantization_lut_lossless_10;
extern std::vector<int8_t> quantization_lut_lossless_12;
extern std::vector<int8_t> quantization_lut_lossless_16;

// Used to determine how large runs should be encoded at a time. Defined by the JPEG-LS standard, A.2.1., Initialization step 3.
constexpr std::array<int, 32> J = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 11, 12, 13, 14, 15};

constexpr int32_t ApplySign(const int32_t i, const int32_t sign) noexcept
{
    return (sign ^ i) - sign;
}


// Two alternatives for GetPredictedValue() (second is slightly faster due to reduced branching)

#if 0

inline int32_t GetPredictedValue(int32_t Ra, int32_t Rb, int32_t Rc)
{
    if (Ra < Rb)
    {
        if (Rc < Ra)
            return Rb;

        if (Rc > Rb)
            return Ra;
    }
    else
    {
        if (Rc < Rb)
            return Ra;

        if (Rc > Ra)
            return Rb;
    }

    return Ra + Rb - Rc;
}

#else

inline int32_t GetPredictedValue(const int32_t Ra, const int32_t Rb, const int32_t Rc) noexcept
{
    // sign trick reduces the number of if statements (branches)
    const int32_t sgn = BitWiseSign(Rb - Ra);

    // is Ra between Rc and Rb?
    if ((sgn ^ (Rc - Ra)) < 0)
    {
        return Rb;
    }
    if ((sgn ^ (Rb - Rc)) < 0)
    {
        return Ra;
    }

    // default case, valid if Rc element of [Ra,Rb]
    return Ra + Rb - Rc;
}

#endif

CONSTEXPR int32_t unmap_error_value(const int32_t mapped_error) noexcept
{
    const int32_t sign = mapped_error << (int32_t_bit_count - 1) >> (int32_t_bit_count - 1);
    return sign ^ (mapped_error >> 1);
}

CONSTEXPR int32_t get_mapped_error_value(const int32_t error_value) noexcept
{
    const int32_t mapped_error = (error_value >> (int32_t_bit_count - 2)) ^ (2 * error_value);
    return mapped_error;
}

constexpr int32_t compute_context_id(const int32_t Q1, const int32_t Q2, const int32_t Q3) noexcept
{
    return (Q1 * 9 + Q2) * 9 + Q3;
}


template<typename Traits, typename Strategy>
class jls_codec final : public Strategy
{
public:
    using PIXEL = typename Traits::PIXEL;
    using SAMPLE = typename Traits::SAMPLE;

    jls_codec(Traits in_traits, const frame_info& frame_info, const coding_parameters& parameters) noexcept :
        Strategy{update_component_count(frame_info, parameters), parameters},
        traits{std::move(in_traits)},
        width_{frame_info.width}
    {
        ASSERT((parameters.interleave_mode == interleave_mode::none && this->frame_info().component_count == 1) || parameters.interleave_mode != interleave_mode::none);
    }

    void set_presets(const jpegls_pc_parameters& presets) override
    {
        const jpegls_pc_parameters presetDefault{compute_default(traits.MAXVAL, traits.NEAR)};

        InitParams(presets.threshold1 != 0 ? presets.threshold1 : presetDefault.threshold1,
                   presets.threshold2 != 0 ? presets.threshold2 : presetDefault.threshold2,
                   presets.threshold3 != 0 ? presets.threshold3 : presetDefault.threshold3,
                   presets.reset_value != 0 ? presets.reset_value : presetDefault.reset_value);
    }

    std::unique_ptr<process_line> create_process(byte_stream_info info, uint32_t stride) override;

    bool is_interleaved() noexcept
    {
        ASSERT((parameters().interleave_mode == interleave_mode::none && frame_info().component_count == 1) || parameters().interleave_mode != interleave_mode::none);

        return parameters().interleave_mode != interleave_mode::none;
    }

    const coding_parameters& parameters() const noexcept
    {
        return Strategy::parameters_;
    }

    const charls::frame_info& frame_info() const noexcept
    {
        return Strategy::frame_info_;
    }

    signed char quantize_gradient_org(int32_t Di) const noexcept;

    FORCE_INLINE int32_t QuantizeGradient(const int32_t Di) const noexcept
    {
        ASSERT(quantize_gradient_org(Di) == *(quantization_ + Di));
        return *(quantization_ + Di);
    }

    void InitQuantizationLUT();

    int32_t DecodeValue(int32_t k, int32_t limit, int32_t qbpp);
    FORCE_INLINE void EncodeMappedValue(int32_t k, int32_t mapped_error, int32_t limit);

    void IncrementRunIndex() noexcept
    {
        RUNindex_ = std::min(31, RUNindex_ + 1);
    }

    void DecrementRunIndex() noexcept
    {
        RUNindex_ = std::max(0, RUNindex_ - 1);
    }

    int32_t DecodeRIError(context_run_mode& ctx);
    triplet<SAMPLE> DecodeRIPixel(triplet<SAMPLE> Ra, triplet<SAMPLE> Rb);
    quad<SAMPLE> DecodeRIPixel(quad<SAMPLE> Ra, quad<SAMPLE> Rb);
    SAMPLE DecodeRIPixel(int32_t Ra, int32_t Rb);
    int32_t DecodeRunPixels(PIXEL Ra, PIXEL* startPos, int32_t cpixelMac);
    int32_t DoRunMode(int32_t start_index, decoder_strategy*);

    void EncodeRIError(context_run_mode& context, int32_t error_value);
    SAMPLE EncodeRIPixel(int32_t x, int32_t Ra, int32_t Rb);
    triplet<SAMPLE> EncodeRIPixel(triplet<SAMPLE> x, triplet<SAMPLE> Ra, triplet<SAMPLE> Rb);
    quad<SAMPLE> EncodeRIPixel(quad<SAMPLE> x, quad<SAMPLE> Ra, quad<SAMPLE> Rb);
    void EncodeRunPixels(int32_t run_length, bool end_of_line);
    int32_t DoRunMode(int32_t index, encoder_strategy*);

    FORCE_INLINE SAMPLE DoRegular(int32_t Qs, int32_t, int32_t pred, decoder_strategy*);
    FORCE_INLINE SAMPLE DoRegular(int32_t Qs, int32_t x, int32_t pred, encoder_strategy*);

    void DoLine(SAMPLE* dummy);
    void DoLine(triplet<SAMPLE>* dummy);
    void DoLine(quad<SAMPLE>* dummy);
    void DoScan();

    void InitParams(int32_t t1, int32_t t2, int32_t t3, int32_t nReset);

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif

    // Note: depending on the base class encode_scan OR decode_scan will be virtual and abstract, cannot use override in all cases.
    // NOLINTNEXTLINE(cppcoreguidelines-explicit-virtual-functions, hicpp-use-override, modernize-use-override)
    size_t encode_scan(std::unique_ptr<process_line> process_line, byte_stream_info& compressed_data);

    // NOLINTNEXTLINE(cppcoreguidelines-explicit-virtual-functions, hicpp-use-override, modernize-use-override)
    void decode_scan(std::unique_ptr<process_line> process_line, const JlsRect& rect, byte_stream_info& compressed_data);

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

private:
    static charls::frame_info update_component_count(charls::frame_info frame, const coding_parameters& parameters) noexcept
    {
        if (parameters.interleave_mode == interleave_mode::none)
        {
            frame.component_count = 1;
        }

        return frame;
    }

    // codec parameters
    Traits traits;
    JlsRect rect_{};
    uint32_t width_;
    int32_t T1{};
    int32_t T2{};
    int32_t T3{};

    // compression context
    std::array<jls_context, 365> contexts_;
    std::array<context_run_mode, 2> contextRunmode_;
    int32_t RUNindex_{};
    PIXEL* previousLine_{};
    PIXEL* currentLine_{};

    // quantization lookup table
    int8_t* quantization_{};
    std::vector<int8_t> quantization_lut_;
};


// Encode/decode a single sample. Performance wise the #1 important functions
template<typename Traits, typename Strategy>
typename Traits::SAMPLE jls_codec<Traits, Strategy>::DoRegular(const int32_t Qs, int32_t, const int32_t pred, decoder_strategy*)
{
    const int32_t sign = BitWiseSign(Qs);
    jls_context& ctx = contexts_[ApplySign(Qs, sign)];
    const int32_t k = ctx.get_golomb_code();
    const int32_t Px = traits.correct_prediction(pred + ApplySign(ctx.C, sign));

    int32_t ErrVal;
    const golomb_code& code = decodingTables[k].get(Strategy::peek_byte());
    if (code.length() != 0)
    {
        Strategy::skip(code.length());
        ErrVal = code.value();
        ASSERT(std::abs(ErrVal) < 65535);
    }
    else
    {
        ErrVal = unmap_error_value(DecodeValue(k, traits.LIMIT, traits.qbpp));
        if (std::abs(ErrVal) > 65535)
            impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);
    }
    if (k == 0)
    {
        ErrVal = ErrVal ^ ctx.get_error_correction(traits.NEAR);
    }
    ctx.update_variables(ErrVal, traits.NEAR, traits.RESET);
    ErrVal = ApplySign(ErrVal, sign);
    return traits.compute_reconstructed_sample(Px, ErrVal);
}


template<typename Traits, typename Strategy>
typename Traits::SAMPLE jls_codec<Traits, Strategy>::DoRegular(const int32_t Qs, int32_t x, const int32_t pred, encoder_strategy*)
{
    const int32_t sign = BitWiseSign(Qs);
    jls_context& ctx = contexts_[ApplySign(Qs, sign)];
    const int32_t k = ctx.get_golomb_code();
    const int32_t Px = traits.correct_prediction(pred + ApplySign(ctx.C, sign));
    const int32_t ErrVal = traits.compute_error_value(ApplySign(x - Px, sign));

    EncodeMappedValue(k, get_mapped_error_value(ctx.get_error_correction(k | traits.NEAR) ^ ErrVal), traits.LIMIT);
    ctx.update_variables(ErrVal, traits.NEAR, traits.RESET);
    ASSERT(traits.is_near(traits.compute_reconstructed_sample(Px, ApplySign(ErrVal, sign)), x));
    return static_cast<SAMPLE>(traits.compute_reconstructed_sample(Px, ApplySign(ErrVal, sign)));
}


// Functions to build tables used to decode short Golomb codes.

inline std::pair<int32_t, int32_t> CreateEncodedValue(const int32_t k, const int32_t mapped_error) noexcept
{
    const int32_t highBits = mapped_error >> k;
    return std::make_pair(highBits + k + 1, (1 << k) | (mapped_error & ((1 << k) - 1)));
}


inline golomb_code_table initialize_table(const int32_t k) noexcept
{
    golomb_code_table table;
    for (short nerr = 0;; ++nerr)
    {
        // Q is not used when k != 0
        const int32_t merrval = get_mapped_error_value(nerr);
        const std::pair<int32_t, int32_t> pairCode = CreateEncodedValue(k, merrval);
        if (static_cast<size_t>(pairCode.first) > golomb_code_table::byte_bit_count)
            break;

        const golomb_code code(nerr, static_cast<short>(pairCode.first));
        table.add_entry(static_cast<uint8_t>(pairCode.second), code);
    }

    for (short nerr = -1;; --nerr)
    {
        // Q is not used when k != 0
        const int32_t merrval = get_mapped_error_value(nerr);
        const std::pair<int32_t, int32_t> pairCode = CreateEncodedValue(k, merrval);
        if (static_cast<size_t>(pairCode.first) > golomb_code_table::byte_bit_count)
            break;

        const golomb_code code = golomb_code(nerr, static_cast<short>(pairCode.first));
        table.add_entry(static_cast<uint8_t>(pairCode.second), code);
    }

    return table;
}


// Encoding/decoding of Golomb codes

template<typename Traits, typename Strategy>
int32_t jls_codec<Traits, Strategy>::DecodeValue(int32_t k, const int32_t limit, int32_t qbpp)
{
    const int32_t highBits = Strategy::read_high_bits();

    if (highBits >= limit - (qbpp + 1))
        return Strategy::read_value(qbpp) + 1;

    if (k == 0)
        return highBits;

    return (highBits << k) + Strategy::read_value(k);
}


template<typename Traits, typename Strategy>
FORCE_INLINE void jls_codec<Traits, Strategy>::EncodeMappedValue(int32_t k, const int32_t mapped_error, int32_t limit)
{
    int32_t highBits = mapped_error >> k;

    if (highBits < limit - traits.qbpp - 1)
    {
        if (highBits + 1 > 31)
        {
            Strategy::append_to_bit_stream(0, highBits / 2);
            highBits = highBits - highBits / 2;
        }
        Strategy::append_to_bit_stream(1, highBits + 1);
        Strategy::append_to_bit_stream((mapped_error & ((1 << k) - 1)), k);
        return;
    }

    if (limit - traits.qbpp > 31)
    {
        Strategy::append_to_bit_stream(0, 31);
        Strategy::append_to_bit_stream(1, limit - traits.qbpp - 31);
    }
    else
    {
        Strategy::append_to_bit_stream(1, limit - traits.qbpp);
    }
    Strategy::append_to_bit_stream((mapped_error - 1) & ((1 << traits.qbpp) - 1), traits.qbpp);
}


// C4127 = conditional expression is constant (caused by some template methods that are not fully specialized) [VS2017]
// 6326 = Potential comparison of a constant with another constant. (false warning, triggered by template construction in Checked build)
// 26814 = The const variable 'RANGE' can be computed at compile-time. [incorrect warning, VS 16.3.0 P3]
MSVC_WARNING_SUPPRESS(4127 6326 26814)

// Sets up a lookup table to "Quantize" sample difference.

template<typename Traits, typename Strategy>
void jls_codec<Traits, Strategy>::InitQuantizationLUT()
{
    // for lossless mode with default parameters, we have precomputed the look up table for bit counts 8, 10, 12 and 16.
    if (traits.NEAR == 0 && traits.MAXVAL == (1 << traits.bpp) - 1)
    {
        const jpegls_pc_parameters presets{compute_default(traits.MAXVAL, traits.NEAR)};
        if (presets.threshold1 == T1 && presets.threshold2 == T2 && presets.threshold3 == T3)
        {
            if (traits.bpp == 8)
            {
                quantization_ = &quantization_lut_lossless_8[quantization_lut_lossless_8.size() / 2];
                return;
            }
            if (traits.bpp == 10)
            {
                quantization_ = &quantization_lut_lossless_10[quantization_lut_lossless_10.size() / 2];
                return;
            }
            if (traits.bpp == 12)
            {
                quantization_ = &quantization_lut_lossless_12[quantization_lut_lossless_12.size() / 2];
                return;
            }
            if (traits.bpp == 16)
            {
                quantization_ = &quantization_lut_lossless_16[quantization_lut_lossless_16.size() / 2];
                return;
            }
        }
    }

    const int32_t range = 1 << traits.bpp;

    quantization_lut_.resize(static_cast<size_t>(range) * 2);

    quantization_ = &quantization_lut_[range];
    for (int32_t i = -range; i < range; ++i)
    {
        quantization_[i] = quantize_gradient_org(i);
    }
}

MSVC_WARNING_UNSUPPRESS()

template<typename Traits, typename Strategy>
signed char jls_codec<Traits, Strategy>::quantize_gradient_org(int32_t Di) const noexcept
{
    if (Di <= -T3) return -4;
    if (Di <= -T2) return -3;
    if (Di <= -T1) return -2;
    if (Di < -traits.NEAR) return -1;
    if (Di <= traits.NEAR) return 0;
    if (Di < T1) return 1;
    if (Di < T2) return 2;
    if (Di < T3) return 3;

    return 4;
}


// RI = Run interruption: functions that handle the sample terminating a run.

template<typename Traits, typename Strategy>
int32_t jls_codec<Traits, Strategy>::DecodeRIError(context_run_mode& ctx)
{
    const int32_t k = ctx.get_golomb_code();
    const int32_t EMErrval = DecodeValue(k, traits.LIMIT - J[RUNindex_] - 1, traits.qbpp);
    const int32_t errorValue = ctx.compute_error_value(EMErrval + ctx.nRItype_, k);
    ctx.update_variables(errorValue, EMErrval);
    return errorValue;
}


template<typename Traits, typename Strategy>
void jls_codec<Traits, Strategy>::EncodeRIError(context_run_mode& context, const int32_t error_value)
{
    const int32_t k = context.get_golomb_code();
    const bool map = context.compute_map(error_value, k);
    const int32_t EMErrval = 2 * std::abs(error_value) - context.nRItype_ - static_cast<int32_t>(map);

    ASSERT(error_value == context.compute_error_value(EMErrval + context.nRItype_, k));
    EncodeMappedValue(k, EMErrval, traits.LIMIT - J[RUNindex_] - 1);
    context.update_variables(error_value, EMErrval);
}


template<typename Traits, typename Strategy>
triplet<typename Traits::SAMPLE> jls_codec<Traits, Strategy>::DecodeRIPixel(triplet<SAMPLE> Ra, triplet<SAMPLE> Rb)
{
    const int32_t errorValue1 = DecodeRIError(contextRunmode_[0]);
    const int32_t errorValue2 = DecodeRIError(contextRunmode_[0]);
    const int32_t errorValue3 = DecodeRIError(contextRunmode_[0]);

    return triplet<SAMPLE>(traits.compute_reconstructed_sample(Rb.v1, errorValue1 * Sign(Rb.v1 - Ra.v1)),
                           traits.compute_reconstructed_sample(Rb.v2, errorValue2 * Sign(Rb.v2 - Ra.v2)),
                           traits.compute_reconstructed_sample(Rb.v3, errorValue3 * Sign(Rb.v3 - Ra.v3)));
}


template<typename Traits, typename Strategy>
triplet<typename Traits::SAMPLE> jls_codec<Traits, Strategy>::EncodeRIPixel(triplet<SAMPLE> x, triplet<SAMPLE> Ra, triplet<SAMPLE> Rb)
{
    const int32_t errorValue1 = traits.compute_error_value(Sign(Rb.v1 - Ra.v1) * (x.v1 - Rb.v1));
    EncodeRIError(contextRunmode_[0], errorValue1);

    const int32_t errorValue2 = traits.compute_error_value(Sign(Rb.v2 - Ra.v2) * (x.v2 - Rb.v2));
    EncodeRIError(contextRunmode_[0], errorValue2);

    const int32_t errorValue3 = traits.compute_error_value(Sign(Rb.v3 - Ra.v3) * (x.v3 - Rb.v3));
    EncodeRIError(contextRunmode_[0], errorValue3);

    return triplet<SAMPLE>(traits.compute_reconstructed_sample(Rb.v1, errorValue1 * Sign(Rb.v1 - Ra.v1)),
                           traits.compute_reconstructed_sample(Rb.v2, errorValue2 * Sign(Rb.v2 - Ra.v2)),
                           traits.compute_reconstructed_sample(Rb.v3, errorValue3 * Sign(Rb.v3 - Ra.v3)));
}

template<typename Traits, typename Strategy>
quad<typename Traits::SAMPLE> jls_codec<Traits, Strategy>::DecodeRIPixel(quad<SAMPLE> Ra, quad<SAMPLE> Rb)
{
    const int32_t errorValue1 = DecodeRIError(contextRunmode_[0]);
    const int32_t errorValue2 = DecodeRIError(contextRunmode_[0]);
    const int32_t errorValue3 = DecodeRIError(contextRunmode_[0]);
    const int32_t errorValue4 = DecodeRIError(contextRunmode_[0]);

    return quad<SAMPLE>(triplet<SAMPLE>(traits.compute_reconstructed_sample(Rb.v1, errorValue1 * Sign(Rb.v1 - Ra.v1)),
                                        traits.compute_reconstructed_sample(Rb.v2, errorValue2 * Sign(Rb.v2 - Ra.v2)),
                                        traits.compute_reconstructed_sample(Rb.v3, errorValue3 * Sign(Rb.v3 - Ra.v3))),
                        traits.compute_reconstructed_sample(Rb.v4, errorValue4 * Sign(Rb.v4 - Ra.v4)));
}


template<typename Traits, typename Strategy>
quad<typename Traits::SAMPLE> jls_codec<Traits, Strategy>::EncodeRIPixel(quad<SAMPLE> x, quad<SAMPLE> Ra, quad<SAMPLE> Rb)
{
    const int32_t errorValue1 = traits.compute_error_value(Sign(Rb.v1 - Ra.v1) * (x.v1 - Rb.v1));
    EncodeRIError(contextRunmode_[0], errorValue1);

    const int32_t errorValue2 = traits.compute_error_value(Sign(Rb.v2 - Ra.v2) * (x.v2 - Rb.v2));
    EncodeRIError(contextRunmode_[0], errorValue2);

    const int32_t errorValue3 = traits.compute_error_value(Sign(Rb.v3 - Ra.v3) * (x.v3 - Rb.v3));
    EncodeRIError(contextRunmode_[0], errorValue3);

    const int32_t errorValue4 = traits.compute_error_value(Sign(Rb.v4 - Ra.v4) * (x.v4 - Rb.v4));
    EncodeRIError(contextRunmode_[0], errorValue4);

    return quad<SAMPLE>(triplet<SAMPLE>(traits.compute_reconstructed_sample(Rb.v1, errorValue1 * Sign(Rb.v1 - Ra.v1)),
                                        traits.compute_reconstructed_sample(Rb.v2, errorValue2 * Sign(Rb.v2 - Ra.v2)),
                                        traits.compute_reconstructed_sample(Rb.v3, errorValue3 * Sign(Rb.v3 - Ra.v3))),
                        traits.compute_reconstructed_sample(Rb.v4, errorValue4 * Sign(Rb.v4 - Ra.v4)));
}


template<typename Traits, typename Strategy>
typename Traits::SAMPLE jls_codec<Traits, Strategy>::DecodeRIPixel(int32_t Ra, int32_t Rb)
{
    if (std::abs(Ra - Rb) <= traits.NEAR)
    {
        const int32_t ErrVal = DecodeRIError(contextRunmode_[1]);
        return static_cast<SAMPLE>(traits.compute_reconstructed_sample(Ra, ErrVal));
    }

    const int32_t ErrVal = DecodeRIError(contextRunmode_[0]);
    return static_cast<SAMPLE>(traits.compute_reconstructed_sample(Rb, ErrVal * Sign(Rb - Ra)));
}


template<typename Traits, typename Strategy>
typename Traits::SAMPLE jls_codec<Traits, Strategy>::EncodeRIPixel(const int32_t x, int32_t Ra, int32_t Rb)
{
    if (std::abs(Ra - Rb) <= traits.NEAR)
    {
        const int32_t ErrVal = traits.compute_error_value(x - Ra);
        EncodeRIError(contextRunmode_[1], ErrVal);
        return static_cast<SAMPLE>(traits.compute_reconstructed_sample(Ra, ErrVal));
    }

    const int32_t ErrVal = traits.compute_error_value((x - Rb) * Sign(Rb - Ra));
    EncodeRIError(contextRunmode_[0], ErrVal);
    return static_cast<SAMPLE>(traits.compute_reconstructed_sample(Rb, ErrVal * Sign(Rb - Ra)));
}


// RunMode: Functions that handle run-length encoding

template<typename Traits, typename Strategy>
void jls_codec<Traits, Strategy>::EncodeRunPixels(int32_t run_length, const bool end_of_line)
{
    while (run_length >= static_cast<int32_t>(1 << J[RUNindex_]))
    {
        Strategy::append_ones_to_bit_stream(1);
        run_length = run_length - static_cast<int32_t>(1 << J[RUNindex_]);
        IncrementRunIndex();
    }

    if (end_of_line)
    {
        if (run_length != 0)
        {
            Strategy::append_ones_to_bit_stream(1);
        }
    }
    else
    {
        Strategy::append_to_bit_stream(run_length, J[RUNindex_] + 1); // leading 0 + actual remaining length
    }
}


template<typename Traits, typename Strategy>
int32_t jls_codec<Traits, Strategy>::DecodeRunPixels(PIXEL Ra, PIXEL* startPos, const int32_t cpixelMac)
{
    int32_t index = 0;
    while (Strategy::read_bit())
    {
        const int count = std::min(1 << J[RUNindex_], static_cast<int>(cpixelMac - index));
        index += count;
        ASSERT(index <= cpixelMac);

        if (count == (1 << J[RUNindex_]))
        {
            IncrementRunIndex();
        }

        if (index == cpixelMac)
            break;
    }

    if (index != cpixelMac)
    {
        // incomplete run.
        index += (J[RUNindex_] > 0) ? Strategy::read_value(J[RUNindex_]) : 0;
    }

    if (index > cpixelMac)
        impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);

    for (int32_t i = 0; i < index; ++i)
    {
        startPos[i] = Ra;
    }

    return index;
}

template<typename Traits, typename Strategy>
int32_t jls_codec<Traits, Strategy>::DoRunMode(int32_t index, encoder_strategy*)
{
    const int32_t ctypeRem = width_ - index;
    PIXEL* ptypeCurX = currentLine_ + index;
    const PIXEL* ptypePrevX = previousLine_ + index;

    const PIXEL Ra = ptypeCurX[-1];

    int32_t runLength = 0;

    while (traits.is_near(ptypeCurX[runLength], Ra))
    {
        ptypeCurX[runLength] = Ra;
        ++runLength;

        if (runLength == ctypeRem)
            break;
    }

    EncodeRunPixels(runLength, runLength == ctypeRem);

    if (runLength == ctypeRem)
        return runLength;

    ptypeCurX[runLength] = EncodeRIPixel(ptypeCurX[runLength], Ra, ptypePrevX[runLength]);
    DecrementRunIndex();
    return runLength + 1;
}


template<typename Traits, typename Strategy>
int32_t jls_codec<Traits, Strategy>::DoRunMode(int32_t start_index, decoder_strategy*)
{
    const PIXEL Ra = currentLine_[start_index - 1];

    const int32_t runLength = DecodeRunPixels(Ra, currentLine_ + start_index, width_ - start_index);
    const uint32_t endIndex = start_index + runLength;

    if (endIndex == width_)
        return endIndex - start_index;

    // run interruption
    const PIXEL Rb = previousLine_[endIndex];
    currentLine_[endIndex] = DecodeRIPixel(Ra, Rb);
    DecrementRunIndex();
    return endIndex - start_index + 1;
}


/// <summary>Encodes/Decodes a scan line of samples</summary>
template<typename Traits, typename Strategy>
void jls_codec<Traits, Strategy>::DoLine(SAMPLE*)
{
    int32_t index = 0;
    int32_t Rb = previousLine_[index - 1];
    int32_t Rd = previousLine_[index];

    while (static_cast<uint32_t>(index) < width_)
    {
        const int32_t Ra = currentLine_[index - 1];
        const int32_t Rc = Rb;
        Rb = Rd;
        Rd = previousLine_[index + 1];

        const int32_t Qs = compute_context_id(QuantizeGradient(Rd - Rb), QuantizeGradient(Rb - Rc), QuantizeGradient(Rc - Ra));

        if (Qs != 0)
        {
            currentLine_[index] = DoRegular(Qs, currentLine_[index], GetPredictedValue(Ra, Rb, Rc), static_cast<Strategy*>(nullptr));
            ++index;
        }
        else
        {
            index += DoRunMode(index, static_cast<Strategy*>(nullptr));
            Rb = previousLine_[index - 1];
            Rd = previousLine_[index];
        }
    }
}


/// <summary>Encodes/Decodes a scan line of triplets in ILV_SAMPLE mode</summary>
template<typename Traits, typename Strategy>
void jls_codec<Traits, Strategy>::DoLine(triplet<SAMPLE>*)
{
    int32_t index = 0;
    while (static_cast<uint32_t>(index) < width_)
    {
        const triplet<SAMPLE> Ra = currentLine_[index - 1];
        const triplet<SAMPLE> Rc = previousLine_[index - 1];
        const triplet<SAMPLE> Rb = previousLine_[index];
        const triplet<SAMPLE> Rd = previousLine_[index + 1];

        const int32_t Qs1 = compute_context_id(QuantizeGradient(Rd.v1 - Rb.v1), QuantizeGradient(Rb.v1 - Rc.v1), QuantizeGradient(Rc.v1 - Ra.v1));
        const int32_t Qs2 = compute_context_id(QuantizeGradient(Rd.v2 - Rb.v2), QuantizeGradient(Rb.v2 - Rc.v2), QuantizeGradient(Rc.v2 - Ra.v2));
        const int32_t Qs3 = compute_context_id(QuantizeGradient(Rd.v3 - Rb.v3), QuantizeGradient(Rb.v3 - Rc.v3), QuantizeGradient(Rc.v3 - Ra.v3));

        if (Qs1 == 0 && Qs2 == 0 && Qs3 == 0)
        {
            index += DoRunMode(index, static_cast<Strategy*>(nullptr));
        }
        else
        {
            triplet<SAMPLE> Rx;
            Rx.v1 = DoRegular(Qs1, currentLine_[index].v1, GetPredictedValue(Ra.v1, Rb.v1, Rc.v1), static_cast<Strategy*>(nullptr));
            Rx.v2 = DoRegular(Qs2, currentLine_[index].v2, GetPredictedValue(Ra.v2, Rb.v2, Rc.v2), static_cast<Strategy*>(nullptr));
            Rx.v3 = DoRegular(Qs3, currentLine_[index].v3, GetPredictedValue(Ra.v3, Rb.v3, Rc.v3), static_cast<Strategy*>(nullptr));
            currentLine_[index] = Rx;
            ++index;
        }
    }
}


/// <summary>Encodes/Decodes a scan line of quads in ILV_SAMPLE mode</summary>
template<typename Traits, typename Strategy>
void jls_codec<Traits, Strategy>::DoLine(quad<SAMPLE>*)
{
    int32_t index = 0;
    while (static_cast<uint32_t>(index) < width_)
    {
        const quad<SAMPLE> Ra = currentLine_[index - 1];
        const quad<SAMPLE> Rc = previousLine_[index - 1];
        const quad<SAMPLE> Rb = previousLine_[index];
        const quad<SAMPLE> Rd = previousLine_[index + 1];

        const int32_t Qs1 = compute_context_id(QuantizeGradient(Rd.v1 - Rb.v1), QuantizeGradient(Rb.v1 - Rc.v1), QuantizeGradient(Rc.v1 - Ra.v1));
        const int32_t Qs2 = compute_context_id(QuantizeGradient(Rd.v2 - Rb.v2), QuantizeGradient(Rb.v2 - Rc.v2), QuantizeGradient(Rc.v2 - Ra.v2));
        const int32_t Qs3 = compute_context_id(QuantizeGradient(Rd.v3 - Rb.v3), QuantizeGradient(Rb.v3 - Rc.v3), QuantizeGradient(Rc.v3 - Ra.v3));
        const int32_t Qs4 = compute_context_id(QuantizeGradient(Rd.v4 - Rb.v4), QuantizeGradient(Rb.v4 - Rc.v4), QuantizeGradient(Rc.v4 - Ra.v4));

        if (Qs1 == 0 && Qs2 == 0 && Qs3 == 0 && Qs4 == 0)
        {
            index += DoRunMode(index, static_cast<Strategy*>(nullptr));
        }
        else
        {
            quad<SAMPLE> Rx;
            Rx.v1 = DoRegular(Qs1, currentLine_[index].v1, GetPredictedValue(Ra.v1, Rb.v1, Rc.v1), static_cast<Strategy*>(nullptr));
            Rx.v2 = DoRegular(Qs2, currentLine_[index].v2, GetPredictedValue(Ra.v2, Rb.v2, Rc.v2), static_cast<Strategy*>(nullptr));
            Rx.v3 = DoRegular(Qs3, currentLine_[index].v3, GetPredictedValue(Ra.v3, Rb.v3, Rc.v3), static_cast<Strategy*>(nullptr));
            Rx.v4 = DoRegular(Qs4, currentLine_[index].v4, GetPredictedValue(Ra.v4, Rb.v4, Rc.v4), static_cast<Strategy*>(nullptr));
            currentLine_[index] = Rx;
            ++index;
        }
    }
}


// DoScan: Encodes or decodes a scan.
// In ILV_SAMPLE mode, multiple components are handled in DoLine
// In ILV_LINE mode, a call do DoLine is made for every component
// In ILV_NONE mode, DoScan is called for each component

template<typename Traits, typename Strategy>
void jls_codec<Traits, Strategy>::DoScan()
{
    const uint32_t pixelStride = width_ + 4U;
    const size_t component_count = parameters().interleave_mode == interleave_mode::line ? static_cast<size_t>(frame_info().component_count) : 1U;

    std::vector<PIXEL> vectmp(static_cast<size_t>(2) * component_count * pixelStride);
    std::vector<int32_t> rgRUNindex(component_count);

    for (uint32_t line = 0; line < frame_info().height; ++line)
    {
        previousLine_ = &vectmp[1];
        currentLine_ = &vectmp[1 + static_cast<size_t>(component_count) * pixelStride];
        if ((line & 1) == 1)
        {
            std::swap(previousLine_, currentLine_);
        }

        Strategy::on_line_begin(width_, currentLine_, pixelStride);

        for (auto component = 0U; component < component_count; ++component)
        {
            RUNindex_ = rgRUNindex[component];

            // initialize edge pixels used for prediction
            previousLine_[width_] = previousLine_[width_ - 1];
            currentLine_[-1] = previousLine_[0];
            DoLine(static_cast<PIXEL*>(nullptr)); // dummy argument for overload resolution

            rgRUNindex[component] = RUNindex_;
            previousLine_ += pixelStride;
            currentLine_ += pixelStride;
        }

        if (static_cast<uint32_t>(rect_.Y) <= line && line < static_cast<uint32_t>(rect_.Y + rect_.Height))
        {
            Strategy::on_line_end(rect_.Width, currentLine_ + rect_.X - (static_cast<size_t>(component_count) * pixelStride), pixelStride);
        }
    }

    Strategy::end_scan();
}


// Factory function for ProcessLine objects to copy/transform un encoded pixels to/from our scan line buffers.
template<typename Traits, typename Strategy>
std::unique_ptr<process_line> jls_codec<Traits, Strategy>::create_process(byte_stream_info info, const uint32_t stride)
{
    if (!is_interleaved())
    {
        return info.rawData ? std::unique_ptr<process_line>(std::make_unique<post_process_single_component>(info.rawData, stride, sizeof(typename Traits::PIXEL))) : std::unique_ptr<process_line>(std::make_unique<post_process_single_stream>(info.rawStream, stride, sizeof(typename Traits::PIXEL)));
    }

    if (parameters().transformation == color_transformation::none)
        return std::make_unique<process_transformed<transform_none<typename Traits::SAMPLE>>>(info, stride, frame_info(), parameters(), transform_none<SAMPLE>());

    if (frame_info().bits_per_sample == sizeof(SAMPLE) * 8)
    {
        switch (parameters().transformation)
        {
        case color_transformation::hp1:
            return std::make_unique<process_transformed<transform_hp1<SAMPLE>>>(info, stride, frame_info(), parameters(), transform_hp1<SAMPLE>());
        case color_transformation::hp2:
            return std::make_unique<process_transformed<transform_hp2<SAMPLE>>>(info, stride, frame_info(), parameters(), transform_hp2<SAMPLE>());
        case color_transformation::hp3:
            return std::make_unique<process_transformed<transform_hp3<SAMPLE>>>(info, stride, frame_info(), parameters(), transform_hp3<SAMPLE>());
        default:
            impl::throw_jpegls_error(jpegls_errc::color_transform_not_supported);
        }
    }

    if (frame_info().bits_per_sample > 8)
    {
        const int shift = 16 - frame_info().bits_per_sample;
        switch (parameters().transformation)
        {
        case color_transformation::hp1:
            return std::make_unique<process_transformed<transform_shifted<transform_hp1<uint16_t>>>>(info, stride, frame_info(), parameters(), transform_shifted<transform_hp1<uint16_t>>(shift));
        case color_transformation::hp2:
            return std::make_unique<process_transformed<transform_shifted<transform_hp2<uint16_t>>>>(info, stride, frame_info(), parameters(), transform_shifted<transform_hp2<uint16_t>>(shift));
        case color_transformation::hp3:
            return std::make_unique<process_transformed<transform_shifted<transform_hp3<uint16_t>>>>(info, stride, frame_info(), parameters(), transform_shifted<transform_hp3<uint16_t>>(shift));
        default:
            impl::throw_jpegls_error(jpegls_errc::color_transform_not_supported);
        }
    }

    impl::throw_jpegls_error(jpegls_errc::bit_depth_for_transform_not_supported);
}


// Setup codec for encoding and calls DoScan
MSVC_WARNING_SUPPRESS(26433) // C.128: Virtual functions should specify exactly one of virtual, override, or final
template<typename Traits, typename Strategy>
size_t jls_codec<Traits, Strategy>::encode_scan(std::unique_ptr<process_line> process_line, byte_stream_info& compressed_data)
{
    Strategy::processLine_ = std::move(process_line);

    Strategy::initialize(compressed_data);
    DoScan();

    return Strategy::get_length();
}


// Setup codec for decoding and calls DoScan
template<typename Traits, typename Strategy>
void jls_codec<Traits, Strategy>::decode_scan(std::unique_ptr<process_line> process_line, const JlsRect& rect, byte_stream_info& compressed_data)
{
    Strategy::processLine_ = std::move(process_line);

    const uint8_t* compressedBytes = compressed_data.rawData;
    rect_ = rect;

    Strategy::initialize(compressed_data);
    DoScan();
    skip_bytes(compressed_data, static_cast<size_t>(Strategy::get_cur_byte_pos() - compressedBytes));
}
MSVC_WARNING_UNSUPPRESS()

// Initialize the codec data structures. Depends on JPEG-LS parameters like Threshold1-Threshold3.
template<typename Traits, typename Strategy>
void jls_codec<Traits, Strategy>::InitParams(const int32_t t1, const int32_t t2, const int32_t t3, const int32_t nReset)
{
    T1 = t1;
    T2 = t2;
    T3 = t3;

    InitQuantizationLUT();

    const jls_context contextInitValue(std::max(2, (traits.RANGE + 32) / 64));
    for (auto& context : contexts_)
    {
        context = contextInitValue;
    }

    contextRunmode_[0] = context_run_mode(std::max(2, (traits.RANGE + 32) / 64), 0, nReset);
    contextRunmode_[1] = context_run_mode(std::max(2, (traits.RANGE + 32) / 64), 1, nReset);
    RUNindex_ = 0;
}

} // namespace charls
