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

constexpr int32_t apply_sign(const int32_t i, const int32_t sign) noexcept
{
    return (sign ^ i) - sign;
}


// Two alternatives for GetPredictedValue() (second is slightly faster due to reduced branching)

#if 0

inline int32_t get_predicted_value(int32_t Ra, int32_t Rb, int32_t Rc)
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

inline int32_t get_predicted_value(const int32_t ra, const int32_t rb, const int32_t rc) noexcept
{
    // sign trick reduces the number of if statements (branches)
    const int32_t sign = bit_wise_sign(rb - ra);

    // is Ra between Rc and Rb?
    if ((sign ^ (rc - ra)) < 0)
    {
        return rb;
    }
    if ((sign ^ (rb - rc)) < 0)
    {
        return ra;
    }

    // default case, valid if Rc element of [Ra,Rb]
    return ra + rb - rc;
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

constexpr int32_t compute_context_id(const int32_t q1, const int32_t q2, const int32_t q3) noexcept
{
    return (q1 * 9 + q2) * 9 + q3;
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
        const jpegls_pc_parameters preset_default{compute_default(traits.MAXVAL, traits.NEAR)};

        InitParams(presets.threshold1 != 0 ? presets.threshold1 : preset_default.threshold1,
                   presets.threshold2 != 0 ? presets.threshold2 : preset_default.threshold2,
                   presets.threshold3 != 0 ? presets.threshold3 : preset_default.threshold3,
                   presets.reset_value != 0 ? presets.reset_value : preset_default.reset_value);
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

    signed char quantize_gradient_org(int32_t di) const noexcept;

    FORCE_INLINE int32_t QuantizeGradient(const int32_t di) const noexcept
    {
        ASSERT(quantize_gradient_org(di) == *(quantization_ + di));
        return *(quantization_ + di);
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

    int32_t DecodeRIError(context_run_mode& context);
    triplet<SAMPLE> DecodeRIPixel(triplet<SAMPLE> ra, triplet<SAMPLE> rb);
    quad<SAMPLE> DecodeRIPixel(quad<SAMPLE> ra, quad<SAMPLE> rb);
    SAMPLE DecodeRIPixel(int32_t ra, int32_t rb);
    int32_t DecodeRunPixels(PIXEL ra, PIXEL* start_pos, int32_t pixel_count);
    int32_t DoRunMode(int32_t start_index, decoder_strategy*);

    void EncodeRIError(context_run_mode& context, int32_t error_value);
    SAMPLE EncodeRIPixel(int32_t x, int32_t ra, int32_t rb);
    triplet<SAMPLE> EncodeRIPixel(triplet<SAMPLE> x, triplet<SAMPLE> ra, triplet<SAMPLE> rb);
    quad<SAMPLE> EncodeRIPixel(quad<SAMPLE> x, quad<SAMPLE> ra, quad<SAMPLE> rb);
    void EncodeRunPixels(int32_t run_length, bool end_of_line);
    int32_t DoRunMode(int32_t index, encoder_strategy*);

    FORCE_INLINE SAMPLE DoRegular(int32_t qs, int32_t, int32_t predicted, decoder_strategy*);
    FORCE_INLINE SAMPLE DoRegular(int32_t qs, int32_t x, int32_t predicted, encoder_strategy*);

    void DoLine(SAMPLE* dummy);
    void DoLine(triplet<SAMPLE>* dummy);
    void DoLine(quad<SAMPLE>* dummy);
    void DoScan();

    void InitParams(int32_t t1, int32_t t2, int32_t t3, int32_t n_reset_threshold);

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
typename Traits::SAMPLE jls_codec<Traits, Strategy>::DoRegular(const int32_t qs, int32_t, const int32_t predicted, decoder_strategy*)
{
    const int32_t sign = bit_wise_sign(qs);
    jls_context& context = contexts_[apply_sign(qs, sign)];
    const int32_t k = context.get_golomb_code();
    const int32_t predicted_value = traits.correct_prediction(predicted + apply_sign(context.C, sign));

    int32_t error_value;
    const golomb_code& code = decodingTables[k].get(Strategy::peek_byte());
    if (code.length() != 0)
    {
        Strategy::skip(code.length());
        error_value = code.value();
        ASSERT(std::abs(error_value) < 65535);
    }
    else
    {
        error_value = unmap_error_value(DecodeValue(k, traits.LIMIT, traits.qbpp));
        if (std::abs(error_value) > 65535)
            impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);
    }
    if (k == 0)
    {
        error_value = error_value ^ context.get_error_correction(traits.NEAR);
    }
    context.update_variables(error_value, traits.NEAR, traits.RESET);
    error_value = apply_sign(error_value, sign);
    return traits.compute_reconstructed_sample(predicted_value, error_value);
}


template<typename Traits, typename Strategy>
typename Traits::SAMPLE jls_codec<Traits, Strategy>::DoRegular(const int32_t qs, int32_t x, const int32_t predicted, encoder_strategy*)
{
    const int32_t sign = bit_wise_sign(qs);
    jls_context& context = contexts_[apply_sign(qs, sign)];
    const int32_t k = context.get_golomb_code();
    const int32_t predicted_value = traits.correct_prediction(predicted + apply_sign(context.C, sign));
    const int32_t error_value = traits.compute_error_value(apply_sign(x - predicted_value, sign));

    EncodeMappedValue(k, get_mapped_error_value(context.get_error_correction(k | traits.NEAR) ^ error_value), traits.LIMIT);
    context.update_variables(error_value, traits.NEAR, traits.RESET);
    ASSERT(traits.is_near(traits.compute_reconstructed_sample(predicted_value, apply_sign(error_value, sign)), x));
    return static_cast<SAMPLE>(traits.compute_reconstructed_sample(predicted_value, apply_sign(error_value, sign)));
}


// Functions to build tables used to decode short Golomb codes.

inline std::pair<int32_t, int32_t> create_encoded_value(const int32_t k, const int32_t mapped_error) noexcept
{
    const int32_t high_bits = mapped_error >> k;
    return std::make_pair(high_bits + k + 1, (1 << k) | (mapped_error & ((1 << k) - 1)));
}


inline golomb_code_table initialize_table(const int32_t k) noexcept
{
    golomb_code_table table;
    for (short nerr = 0;; ++nerr)
    {
        // Q is not used when k != 0
        const int32_t mapped_error_value = get_mapped_error_value(nerr);
        const std::pair<int32_t, int32_t> pair_code = create_encoded_value(k, mapped_error_value);
        if (static_cast<size_t>(pair_code.first) > golomb_code_table::byte_bit_count)
            break;

        const golomb_code code(nerr, static_cast<short>(pair_code.first));
        table.add_entry(static_cast<uint8_t>(pair_code.second), code);
    }

    for (short nerr = -1;; --nerr)
    {
        // Q is not used when k != 0
        const int32_t mapped_error_value = get_mapped_error_value(nerr);
        const std::pair<int32_t, int32_t> pair_code = create_encoded_value(k, mapped_error_value);
        if (static_cast<size_t>(pair_code.first) > golomb_code_table::byte_bit_count)
            break;

        const golomb_code code = golomb_code(nerr, static_cast<short>(pair_code.first));
        table.add_entry(static_cast<uint8_t>(pair_code.second), code);
    }

    return table;
}


// Encoding/decoding of Golomb codes

template<typename Traits, typename Strategy>
int32_t jls_codec<Traits, Strategy>::DecodeValue(int32_t k, const int32_t limit, int32_t qbpp)
{
    const int32_t high_bits = Strategy::read_high_bits();

    if (high_bits >= limit - (qbpp + 1))
        return Strategy::read_value(qbpp) + 1;

    if (k == 0)
        return high_bits;

    return (high_bits << k) + Strategy::read_value(k);
}


template<typename Traits, typename Strategy>
FORCE_INLINE void jls_codec<Traits, Strategy>::EncodeMappedValue(int32_t k, const int32_t mapped_error, int32_t limit)
{
    int32_t high_bits = mapped_error >> k;

    if (high_bits < limit - traits.qbpp - 1)
    {
        if (high_bits + 1 > 31)
        {
            Strategy::append_to_bit_stream(0, high_bits / 2);
            high_bits = high_bits - high_bits / 2;
        }
        Strategy::append_to_bit_stream(1, high_bits + 1);
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
signed char jls_codec<Traits, Strategy>::quantize_gradient_org(int32_t di) const noexcept
{
    if (di <= -T3) return -4;
    if (di <= -T2) return -3;
    if (di <= -T1) return -2;
    if (di < -traits.NEAR) return -1;
    if (di <= traits.NEAR) return 0;
    if (di < T1) return 1;
    if (di < T2) return 2;
    if (di < T3) return 3;

    return 4;
}


// RI = Run interruption: functions that handle the sample terminating of a run.

template<typename Traits, typename Strategy>
int32_t jls_codec<Traits, Strategy>::DecodeRIError(context_run_mode& context)
{
    const int32_t k = context.get_golomb_code();
    const int32_t e_mapped_error_value = DecodeValue(k, traits.LIMIT - J[RUNindex_] - 1, traits.qbpp);
    const int32_t error_value = context.compute_error_value(e_mapped_error_value + context.nRItype_, k);
    context.update_variables(error_value, e_mapped_error_value);
    return error_value;
}


template<typename Traits, typename Strategy>
void jls_codec<Traits, Strategy>::EncodeRIError(context_run_mode& context, const int32_t error_value)
{
    const int32_t k = context.get_golomb_code();
    const bool map = context.compute_map(error_value, k);
    const int32_t e_mapped_error_value = 2 * std::abs(error_value) - context.nRItype_ - static_cast<int32_t>(map);

    ASSERT(error_value == context.compute_error_value(e_mapped_error_value + context.nRItype_, k));
    EncodeMappedValue(k, e_mapped_error_value, traits.LIMIT - J[RUNindex_] - 1);
    context.update_variables(error_value, e_mapped_error_value);
}


template<typename Traits, typename Strategy>
triplet<typename Traits::SAMPLE> jls_codec<Traits, Strategy>::DecodeRIPixel(triplet<SAMPLE> ra, triplet<SAMPLE> rb)
{
    const int32_t error_value1 = DecodeRIError(contextRunmode_[0]);
    const int32_t error_value2 = DecodeRIError(contextRunmode_[0]);
    const int32_t error_value3 = DecodeRIError(contextRunmode_[0]);

    return triplet<SAMPLE>(traits.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                           traits.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                           traits.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3)));
}


template<typename Traits, typename Strategy>
triplet<typename Traits::SAMPLE> jls_codec<Traits, Strategy>::EncodeRIPixel(triplet<SAMPLE> x, triplet<SAMPLE> ra, triplet<SAMPLE> rb)
{
    const int32_t error_value1 = traits.compute_error_value(sign(rb.v1 - ra.v1) * (x.v1 - rb.v1));
    EncodeRIError(contextRunmode_[0], error_value1);

    const int32_t error_value2 = traits.compute_error_value(sign(rb.v2 - ra.v2) * (x.v2 - rb.v2));
    EncodeRIError(contextRunmode_[0], error_value2);

    const int32_t error_value3 = traits.compute_error_value(sign(rb.v3 - ra.v3) * (x.v3 - rb.v3));
    EncodeRIError(contextRunmode_[0], error_value3);

    return triplet<SAMPLE>(traits.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                           traits.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                           traits.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3)));
}

template<typename Traits, typename Strategy>
quad<typename Traits::SAMPLE> jls_codec<Traits, Strategy>::DecodeRIPixel(quad<SAMPLE> ra, quad<SAMPLE> rb)
{
    const int32_t error_value1 = DecodeRIError(contextRunmode_[0]);
    const int32_t error_value2 = DecodeRIError(contextRunmode_[0]);
    const int32_t error_value3 = DecodeRIError(contextRunmode_[0]);
    const int32_t error_value4 = DecodeRIError(contextRunmode_[0]);

    return quad<SAMPLE>(triplet<SAMPLE>(traits.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                                        traits.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                                        traits.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3))),
                        traits.compute_reconstructed_sample(rb.v4, error_value4 * sign(rb.v4 - ra.v4)));
}


template<typename Traits, typename Strategy>
quad<typename Traits::SAMPLE> jls_codec<Traits, Strategy>::EncodeRIPixel(quad<SAMPLE> x, quad<SAMPLE> ra, quad<SAMPLE> rb)
{
    const int32_t error_value1 = traits.compute_error_value(sign(rb.v1 - ra.v1) * (x.v1 - rb.v1));
    EncodeRIError(contextRunmode_[0], error_value1);

    const int32_t error_value2 = traits.compute_error_value(sign(rb.v2 - ra.v2) * (x.v2 - rb.v2));
    EncodeRIError(contextRunmode_[0], error_value2);

    const int32_t error_value3 = traits.compute_error_value(sign(rb.v3 - ra.v3) * (x.v3 - rb.v3));
    EncodeRIError(contextRunmode_[0], error_value3);

    const int32_t error_value4 = traits.compute_error_value(sign(rb.v4 - ra.v4) * (x.v4 - rb.v4));
    EncodeRIError(contextRunmode_[0], error_value4);

    return quad<SAMPLE>(triplet<SAMPLE>(traits.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                                        traits.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                                        traits.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3))),
                        traits.compute_reconstructed_sample(rb.v4, error_value4 * sign(rb.v4 - ra.v4)));
}


template<typename Traits, typename Strategy>
typename Traits::SAMPLE jls_codec<Traits, Strategy>::DecodeRIPixel(int32_t ra, int32_t rb)
{
    if (std::abs(ra - rb) <= traits.NEAR)
    {
        const int32_t error_value = DecodeRIError(contextRunmode_[1]);
        return static_cast<SAMPLE>(traits.compute_reconstructed_sample(ra, error_value));
    }

    const int32_t error_value = DecodeRIError(contextRunmode_[0]);
    return static_cast<SAMPLE>(traits.compute_reconstructed_sample(rb, error_value * sign(rb - ra)));
}


template<typename Traits, typename Strategy>
typename Traits::SAMPLE jls_codec<Traits, Strategy>::EncodeRIPixel(const int32_t x, int32_t ra, int32_t rb)
{
    if (std::abs(ra - rb) <= traits.NEAR)
    {
        const int32_t error_value = traits.compute_error_value(x - ra);
        EncodeRIError(contextRunmode_[1], error_value);
        return static_cast<SAMPLE>(traits.compute_reconstructed_sample(ra, error_value));
    }

    const int32_t error_value = traits.compute_error_value((x - rb) * sign(rb - ra));
    EncodeRIError(contextRunmode_[0], error_value);
    return static_cast<SAMPLE>(traits.compute_reconstructed_sample(rb, error_value * sign(rb - ra)));
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
int32_t jls_codec<Traits, Strategy>::DecodeRunPixels(PIXEL ra, PIXEL* start_pos, const int32_t pixel_count)
{
    int32_t index = 0;
    while (Strategy::read_bit())
    {
        const int count = std::min(1 << J[RUNindex_], static_cast<int>(pixel_count - index));
        index += count;
        ASSERT(index <= pixel_count);

        if (count == (1 << J[RUNindex_]))
        {
            IncrementRunIndex();
        }

        if (index == pixel_count)
            break;
    }

    if (index != pixel_count)
    {
        // incomplete run.
        index += (J[RUNindex_] > 0) ? Strategy::read_value(J[RUNindex_]) : 0;
    }

    if (index > pixel_count)
        impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);

    for (int32_t i = 0; i < index; ++i)
    {
        start_pos[i] = ra;
    }

    return index;
}

template<typename Traits, typename Strategy>
int32_t jls_codec<Traits, Strategy>::DoRunMode(int32_t index, encoder_strategy*)
{
    const int32_t ctypeRem = width_ - index;
    PIXEL* type_cur_x = currentLine_ + index;
    const PIXEL* type_prev_x = previousLine_ + index;

    const PIXEL ra = type_cur_x[-1];

    int32_t run_length{};
    while (traits.is_near(type_cur_x[run_length], ra))
    {
        type_cur_x[run_length] = ra;
        ++run_length;

        if (run_length == ctypeRem)
            break;
    }

    EncodeRunPixels(run_length, run_length == ctypeRem);

    if (run_length == ctypeRem)
        return run_length;

    type_cur_x[run_length] = EncodeRIPixel(type_cur_x[run_length], ra, type_prev_x[run_length]);
    DecrementRunIndex();
    return run_length + 1;
}


template<typename Traits, typename Strategy>
int32_t jls_codec<Traits, Strategy>::DoRunMode(int32_t start_index, decoder_strategy*)
{
    const PIXEL ra = currentLine_[start_index - 1];

    const int32_t run_length = DecodeRunPixels(ra, currentLine_ + start_index, width_ - start_index);
    const uint32_t end_index = start_index + run_length;

    if (end_index == width_)
        return end_index - start_index;

    // run interruption
    const PIXEL rb = previousLine_[end_index];
    currentLine_[end_index] = DecodeRIPixel(ra, rb);
    DecrementRunIndex();
    return end_index - start_index + 1;
}


/// <summary>Encodes/Decodes a scan line of samples</summary>
template<typename Traits, typename Strategy>
void jls_codec<Traits, Strategy>::DoLine(SAMPLE*)
{
    int32_t index = 0;
    int32_t rb = previousLine_[index - 1];
    int32_t rd = previousLine_[index];

    while (static_cast<uint32_t>(index) < width_)
    {
        const int32_t ra = currentLine_[index - 1];
        const int32_t rc = rb;
        rb = rd;
        rd = previousLine_[index + 1];

        const int32_t qs = compute_context_id(QuantizeGradient(rd - rb), QuantizeGradient(rb - rc), QuantizeGradient(rc - ra));

        if (qs != 0)
        {
            currentLine_[index] = DoRegular(qs, currentLine_[index], get_predicted_value(ra, rb, rc), static_cast<Strategy*>(nullptr));
            ++index;
        }
        else
        {
            index += DoRunMode(index, static_cast<Strategy*>(nullptr));
            rb = previousLine_[index - 1];
            rd = previousLine_[index];
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
        const triplet<SAMPLE> ra = currentLine_[index - 1];
        const triplet<SAMPLE> rc = previousLine_[index - 1];
        const triplet<SAMPLE> rb = previousLine_[index];
        const triplet<SAMPLE> rd = previousLine_[index + 1];

        const int32_t qs1 = compute_context_id(QuantizeGradient(rd.v1 - rb.v1), QuantizeGradient(rb.v1 - rc.v1), QuantizeGradient(rc.v1 - ra.v1));
        const int32_t qs2 = compute_context_id(QuantizeGradient(rd.v2 - rb.v2), QuantizeGradient(rb.v2 - rc.v2), QuantizeGradient(rc.v2 - ra.v2));
        const int32_t qs3 = compute_context_id(QuantizeGradient(rd.v3 - rb.v3), QuantizeGradient(rb.v3 - rc.v3), QuantizeGradient(rc.v3 - ra.v3));

        if (qs1 == 0 && qs2 == 0 && qs3 == 0)
        {
            index += DoRunMode(index, static_cast<Strategy*>(nullptr));
        }
        else
        {
            triplet<SAMPLE> rx;
            rx.v1 = DoRegular(qs1, currentLine_[index].v1, get_predicted_value(ra.v1, rb.v1, rc.v1), static_cast<Strategy*>(nullptr));
            rx.v2 = DoRegular(qs2, currentLine_[index].v2, get_predicted_value(ra.v2, rb.v2, rc.v2), static_cast<Strategy*>(nullptr));
            rx.v3 = DoRegular(qs3, currentLine_[index].v3, get_predicted_value(ra.v3, rb.v3, rc.v3), static_cast<Strategy*>(nullptr));
            currentLine_[index] = rx;
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
        const quad<SAMPLE> ra = currentLine_[index - 1];
        const quad<SAMPLE> rc = previousLine_[index - 1];
        const quad<SAMPLE> rb = previousLine_[index];
        const quad<SAMPLE> rd = previousLine_[index + 1];

        const int32_t qs1 = compute_context_id(QuantizeGradient(rd.v1 - rb.v1), QuantizeGradient(rb.v1 - rc.v1), QuantizeGradient(rc.v1 - ra.v1));
        const int32_t qs2 = compute_context_id(QuantizeGradient(rd.v2 - rb.v2), QuantizeGradient(rb.v2 - rc.v2), QuantizeGradient(rc.v2 - ra.v2));
        const int32_t qs3 = compute_context_id(QuantizeGradient(rd.v3 - rb.v3), QuantizeGradient(rb.v3 - rc.v3), QuantizeGradient(rc.v3 - ra.v3));
        const int32_t qs4 = compute_context_id(QuantizeGradient(rd.v4 - rb.v4), QuantizeGradient(rb.v4 - rc.v4), QuantizeGradient(rc.v4 - ra.v4));

        if (qs1 == 0 && qs2 == 0 && qs3 == 0 && qs4 == 0)
        {
            index += DoRunMode(index, static_cast<Strategy*>(nullptr));
        }
        else
        {
            quad<SAMPLE> rx;
            rx.v1 = DoRegular(qs1, currentLine_[index].v1, get_predicted_value(ra.v1, rb.v1, rc.v1), static_cast<Strategy*>(nullptr));
            rx.v2 = DoRegular(qs2, currentLine_[index].v2, get_predicted_value(ra.v2, rb.v2, rc.v2), static_cast<Strategy*>(nullptr));
            rx.v3 = DoRegular(qs3, currentLine_[index].v3, get_predicted_value(ra.v3, rb.v3, rc.v3), static_cast<Strategy*>(nullptr));
            rx.v4 = DoRegular(qs4, currentLine_[index].v4, get_predicted_value(ra.v4, rb.v4, rc.v4), static_cast<Strategy*>(nullptr));
            currentLine_[index] = rx;
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
    const uint32_t pixel_stride = width_ + 4U;
    const size_t component_count = parameters().interleave_mode == interleave_mode::line ? static_cast<size_t>(frame_info().component_count) : 1U;

    std::vector<PIXEL> vectmp(static_cast<size_t>(2) * component_count * pixel_stride);
    std::vector<int32_t> run_index(component_count);

    for (uint32_t line = 0; line < frame_info().height; ++line)
    {
        previousLine_ = &vectmp[1];
        currentLine_ = &vectmp[1 + static_cast<size_t>(component_count) * pixel_stride];
        if ((line & 1) == 1)
        {
            std::swap(previousLine_, currentLine_);
        }

        Strategy::on_line_begin(width_, currentLine_, pixel_stride);

        for (auto component = 0U; component < component_count; ++component)
        {
            RUNindex_ = run_index[component];

            // initialize edge pixels used for prediction
            previousLine_[width_] = previousLine_[width_ - 1];
            currentLine_[-1] = previousLine_[0];
            DoLine(static_cast<PIXEL*>(nullptr)); // dummy argument for overload resolution

            run_index[component] = RUNindex_;
            previousLine_ += pixel_stride;
            currentLine_ += pixel_stride;
        }

        if (static_cast<uint32_t>(rect_.Y) <= line && line < static_cast<uint32_t>(rect_.Y + rect_.Height))
        {
            Strategy::on_line_end(rect_.Width, currentLine_ + rect_.X - (static_cast<size_t>(component_count) * pixel_stride), pixel_stride);
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

    const uint8_t* compressed_bytes = compressed_data.rawData;
    rect_ = rect;

    Strategy::initialize(compressed_data);
    DoScan();
    skip_bytes(compressed_data, static_cast<size_t>(Strategy::get_cur_byte_pos() - compressed_bytes));
}
MSVC_WARNING_UNSUPPRESS()

// Initialize the codec data structures. Depends on JPEG-LS parameters like Threshold1-Threshold3.
template<typename Traits, typename Strategy>
void jls_codec<Traits, Strategy>::InitParams(const int32_t t1, const int32_t t2, const int32_t t3, const int32_t n_reset_threshold)
{
    T1 = t1;
    T2 = t2;
    T3 = t3;

    InitQuantizationLUT();

    const jls_context context_init_value(std::max(2, (traits.RANGE + 32) / 64));
    for (auto& context : contexts_)
    {
        context = context_init_value;
    }

    contextRunmode_[0] = context_run_mode(std::max(2, (traits.RANGE + 32) / 64), 0, n_reset_threshold);
    contextRunmode_[1] = context_run_mode(std::max(2, (traits.RANGE + 32) / 64), 1, n_reset_threshold);
    RUNindex_ = 0;
}

} // namespace charls
