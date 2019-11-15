// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "color_transform.h"
#include "context.h"
#include "context_run_mode.h"
#include "lookup_table.h"
#include "process_line.h"

#include <array>
#include <sstream>

// This file contains the code for handling a "scan". Usually an image is encoded as a single scan.

namespace charls {

extern CTable decodingTables[16];
extern std::vector<signed char> rgquant8Ll;
extern std::vector<signed char> rgquant10Ll;
extern std::vector<signed char> rgquant12Ll;
extern std::vector<signed char> rgquant16Ll;

constexpr int32_t ApplySign(int32_t i, int32_t sign) noexcept
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

inline int32_t GetPredictedValue(int32_t Ra, int32_t Rb, int32_t Rc) noexcept
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

constexpr int32_t UnMapErrVal(int32_t mappedError) noexcept
{
    const int32_t sign = mappedError << (int32_t_bit_count - 1) >> (int32_t_bit_count - 1);
    return sign ^ (mappedError >> 1);
}

constexpr int32_t GetMappedErrVal(int32_t errorValue) noexcept
{
    const int32_t mappedError = (errorValue >> (int32_t_bit_count - 2)) ^ (2 * errorValue);
    return mappedError;
}

constexpr int32_t ComputeContextID(int32_t Q1, int32_t Q2, int32_t Q3) noexcept
{
    return (Q1 * 9 + Q2) * 9 + Q3;
}


template<typename Traits, typename Strategy>
class JlsCodec final : public Strategy
{
public:
    using PIXEL = typename Traits::PIXEL;
    using SAMPLE = typename Traits::SAMPLE;

    JlsCodec(Traits inTraits, const JlsParameters& params) :
        Strategy{params},
        traits{std::move(inTraits)},
        width_{params.width}
    {
        if (Info().interleaveMode == interleave_mode::none)
        {
            Info().components = 1;
        }
    }

    void SetPresets(const jpegls_pc_parameters& presets) override
    {
        const jpegls_pc_parameters presetDefault{compute_default(traits.MAXVAL, traits.NEAR)};

        InitParams(presets.threshold1 != 0 ? presets.threshold1 : presetDefault.threshold1,
                   presets.threshold2 != 0 ? presets.threshold2 : presetDefault.threshold2,
                   presets.threshold3 != 0 ? presets.threshold3 : presetDefault.threshold3,
                   presets.reset_value != 0 ? presets.reset_value : presetDefault.reset_value);
    }

    std::unique_ptr<ProcessLine> CreateProcess(ByteStreamInfo info) override;

    bool IsInterleaved() noexcept
    {
        if (Info().interleaveMode == interleave_mode::none)
            return false;

        if (Info().components == 1)
            return false;

        return true;
    }

    JlsParameters& Info() noexcept
    {
        return Strategy::params_;
    }

    signed char QuantizeGradientOrg(int32_t Di) const noexcept;

    FORCE_INLINE int32_t QuantizeGradient(int32_t Di) const noexcept
    {
        ASSERT(QuantizeGradientOrg(Di) == *(pquant_ + Di));
        return *(pquant_ + Di);
    }

    void InitQuantizationLUT();

    int32_t DecodeValue(int32_t k, int32_t limit, int32_t qbpp);
    FORCE_INLINE void EncodeMappedValue(int32_t k, int32_t mappedError, int32_t limit);

    void IncrementRunIndex() noexcept
    {
        RUNindex_ = std::min(31, RUNindex_ + 1);
    }

    void DecrementRunIndex() noexcept
    {
        RUNindex_ = std::max(0, RUNindex_ - 1);
    }

    int32_t DecodeRIError(CContextRunMode& ctx);
    Triplet<SAMPLE> DecodeRIPixel(Triplet<SAMPLE> Ra, Triplet<SAMPLE> Rb);
    Quad<SAMPLE> DecodeRIPixel(Quad<SAMPLE> Ra, Quad<SAMPLE> Rb);
    SAMPLE DecodeRIPixel(int32_t Ra, int32_t Rb);
    int32_t DecodeRunPixels(PIXEL Ra, PIXEL* startPos, int32_t cpixelMac);
    int32_t DoRunMode(int32_t startIndex, DecoderStrategy*);

    void EncodeRIError(CContextRunMode& ctx, int32_t errorValue);
    SAMPLE EncodeRIPixel(int32_t x, int32_t Ra, int32_t Rb);
    Triplet<SAMPLE> EncodeRIPixel(Triplet<SAMPLE> x, Triplet<SAMPLE> Ra, Triplet<SAMPLE> Rb);
    Quad<SAMPLE> EncodeRIPixel(Quad<SAMPLE> x, Quad<SAMPLE> Ra, Quad<SAMPLE> Rb);
    void EncodeRunPixels(int32_t runLength, bool endOfLine);
    int32_t DoRunMode(int32_t index, EncoderStrategy*);

    FORCE_INLINE SAMPLE DoRegular(int32_t Qs, int32_t, int32_t pred, DecoderStrategy*);
    FORCE_INLINE SAMPLE DoRegular(int32_t Qs, int32_t x, int32_t pred, EncoderStrategy*);

    void DoLine(SAMPLE* dummy);
    void DoLine(Triplet<SAMPLE>* dummy);
    void DoLine(Quad<SAMPLE>* dummy);
    void DoScan();

    void InitParams(int32_t t1, int32_t t2, int32_t t3, int32_t nReset);

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif

    // Note: depending on the base class EncodeScan OR DecodeScan will be virtual and abstract, cannot use override in all cases.
    size_t EncodeScan(std::unique_ptr<ProcessLine> processLine, ByteStreamInfo& compressedData);
    void DecodeScan(std::unique_ptr<ProcessLine> processLine, const JlsRect& rect, ByteStreamInfo& compressedData);

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

protected:
    // codec parameters
    Traits traits;
    JlsRect rect_{};
    int width_;
    int32_t T1{};
    int32_t T2{};
    int32_t T3{};

    // compression context
    std::array<JlsContext, 365> contexts_;
    std::array<CContextRunMode, 2> contextRunmode_;
    int32_t RUNindex_{};
    PIXEL* previousLine_{};
    PIXEL* currentLine_{};

    // quantization lookup table
    signed char* pquant_{};
    std::vector<signed char> rgquant_;
};


// Encode/decode a single sample. Performance wise the #1 important functions
template<typename Traits, typename Strategy>
typename Traits::SAMPLE JlsCodec<Traits, Strategy>::DoRegular(int32_t Qs, int32_t, int32_t pred, DecoderStrategy*)
{
    const int32_t sign = BitWiseSign(Qs);
    JlsContext& ctx = contexts_[ApplySign(Qs, sign)];
    const int32_t k = ctx.GetGolomb();
    const int32_t Px = traits.CorrectPrediction(pred + ApplySign(ctx.C, sign));

    int32_t ErrVal;
    const Code& code = decodingTables[k].Get(Strategy::PeekByte());
    if (code.GetLength() != 0)
    {
        Strategy::Skip(code.GetLength());
        ErrVal = code.GetValue();
        ASSERT(std::abs(ErrVal) < 65535);
    }
    else
    {
        ErrVal = UnMapErrVal(DecodeValue(k, traits.LIMIT, traits.qbpp));
        if (std::abs(ErrVal) > 65535)
            throw jpegls_error{jpegls_errc::invalid_encoded_data};
    }
    if (k == 0)
    {
        ErrVal = ErrVal ^ ctx.GetErrorCorrection(traits.NEAR);
    }
    ctx.UpdateVariables(ErrVal, traits.NEAR, traits.RESET);
    ErrVal = ApplySign(ErrVal, sign);
    return traits.ComputeReconstructedSample(Px, ErrVal);
}


template<typename Traits, typename Strategy>
typename Traits::SAMPLE JlsCodec<Traits, Strategy>::DoRegular(int32_t Qs, int32_t x, int32_t pred, EncoderStrategy*)
{
    const int32_t sign = BitWiseSign(Qs);
    JlsContext& ctx = contexts_[ApplySign(Qs, sign)];
    const int32_t k = ctx.GetGolomb();
    const int32_t Px = traits.CorrectPrediction(pred + ApplySign(ctx.C, sign));
    const int32_t ErrVal = traits.ComputeErrVal(ApplySign(x - Px, sign));

    EncodeMappedValue(k, GetMappedErrVal(ctx.GetErrorCorrection(k | traits.NEAR) ^ ErrVal), traits.LIMIT);
    ctx.UpdateVariables(ErrVal, traits.NEAR, traits.RESET);
    ASSERT(traits.IsNear(traits.ComputeReconstructedSample(Px, ApplySign(ErrVal, sign)), x));
    return static_cast<SAMPLE>(traits.ComputeReconstructedSample(Px, ApplySign(ErrVal, sign)));
}


// Functions to build tables used to decode short Golomb codes.

inline std::pair<int32_t, int32_t> CreateEncodedValue(int32_t k, int32_t mappedError) noexcept
{
    const int32_t highBits = mappedError >> k;
    return std::make_pair(highBits + k + 1, (1 << k) | (mappedError & ((1 << k) - 1)));
}


inline CTable InitTable(int32_t k) noexcept
{
    CTable table;
    for (short nerr = 0;; nerr++)
    {
        // Q is not used when k != 0
        const int32_t merrval = GetMappedErrVal(nerr);
        const std::pair<int32_t, int32_t> pairCode = CreateEncodedValue(k, merrval);
        if (static_cast<size_t>(pairCode.first) > CTable::byte_bit_count)
            break;

        const Code code(nerr, static_cast<short>(pairCode.first));
        table.AddEntry(static_cast<uint8_t>(pairCode.second), code);
    }

    for (short nerr = -1;; nerr--)
    {
        // Q is not used when k != 0
        const int32_t merrval = GetMappedErrVal(nerr);
        const std::pair<int32_t, int32_t> pairCode = CreateEncodedValue(k, merrval);
        if (static_cast<size_t>(pairCode.first) > CTable::byte_bit_count)
            break;

        const Code code = Code(nerr, static_cast<short>(pairCode.first));
        table.AddEntry(static_cast<uint8_t>(pairCode.second), code);
    }

    return table;
}


// Encoding/decoding of Golomb codes

template<typename Traits, typename Strategy>
int32_t JlsCodec<Traits, Strategy>::DecodeValue(int32_t k, int32_t limit, int32_t qbpp)
{
    const int32_t highBits = Strategy::ReadHighBits();

    if (highBits >= limit - (qbpp + 1))
        return Strategy::ReadValue(qbpp) + 1;

    if (k == 0)
        return highBits;

    return (highBits << k) + Strategy::ReadValue(k);
}


template<typename Traits, typename Strategy>
FORCE_INLINE void JlsCodec<Traits, Strategy>::EncodeMappedValue(int32_t k, int32_t mappedError, int32_t limit)
{
    int32_t highBits = mappedError >> k;

    if (highBits < limit - traits.qbpp - 1)
    {
        if (highBits + 1 > 31)
        {
            Strategy::AppendToBitStream(0, highBits / 2);
            highBits = highBits - highBits / 2;
        }
        Strategy::AppendToBitStream(1, highBits + 1);
        Strategy::AppendToBitStream((mappedError & ((1 << k) - 1)), k);
        return;
    }

    if (limit - traits.qbpp > 31)
    {
        Strategy::AppendToBitStream(0, 31);
        Strategy::AppendToBitStream(1, limit - traits.qbpp - 31);
    }
    else
    {
        Strategy::AppendToBitStream(1, limit - traits.qbpp);
    }
    Strategy::AppendToBitStream((mappedError - 1) & ((1 << traits.qbpp) - 1), traits.qbpp);
}


// C4127 = conditional expression is constant (caused by some template methods that are not fully specialized) [VS2017]
// 6326 = Potential comparison of a constant with another constant. (false warning, triggered by template construction in Checked build)
// 26814 = The const variable 'RANGE' can be computed at compile-time. [incorrect warning, VS 16.3.0 P3]
MSVC_WARNING_SUPPRESS(4127 6326 26814)

// Sets up a lookup table to "Quantize" sample difference.

template<typename Traits, typename Strategy>
void JlsCodec<Traits, Strategy>::InitQuantizationLUT()
{
    // for lossless mode with default parameters, we have precomputed the look up table for bit counts 8, 10, 12 and 16.
    if (traits.NEAR == 0 && traits.MAXVAL == (1 << traits.bpp) - 1)
    {
        const jpegls_pc_parameters presets{compute_default(traits.MAXVAL, traits.NEAR)};
        if (presets.threshold1 == T1 && presets.threshold2 == T2 && presets.threshold3 == T3)
        {
            if (traits.bpp == 8)
            {
                pquant_ = &rgquant8Ll[rgquant8Ll.size() / 2];
                return;
            }
            if (traits.bpp == 10)
            {
                pquant_ = &rgquant10Ll[rgquant10Ll.size() / 2];
                return;
            }
            if (traits.bpp == 12)
            {
                pquant_ = &rgquant12Ll[rgquant12Ll.size() / 2];
                return;
            }
            if (traits.bpp == 16)
            {
                pquant_ = &rgquant16Ll[rgquant16Ll.size() / 2];
                return;
            }
        }
    }

    const int32_t RANGE = 1 << traits.bpp;

    rgquant_.resize(static_cast<size_t>(RANGE) * 2);

    pquant_ = &rgquant_[RANGE];
    for (int32_t i = -RANGE; i < RANGE; ++i)
    {
        pquant_[i] = QuantizeGradientOrg(i);
    }
}

MSVC_WARNING_UNSUPPRESS()

template<typename Traits, typename Strategy>
signed char JlsCodec<Traits, Strategy>::QuantizeGradientOrg(int32_t Di) const noexcept
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
int32_t JlsCodec<Traits, Strategy>::DecodeRIError(CContextRunMode& ctx)
{
    const int32_t k = ctx.GetGolomb();
    const int32_t EMErrval = DecodeValue(k, traits.LIMIT - J[RUNindex_] - 1, traits.qbpp);
    const int32_t errorValue = ctx.ComputeErrVal(EMErrval + ctx.nRItype_, k);
    ctx.UpdateVariables(errorValue, EMErrval);
    return errorValue;
}


template<typename Traits, typename Strategy>
void JlsCodec<Traits, Strategy>::EncodeRIError(CContextRunMode& ctx, int32_t errorValue)
{
    const int32_t k = ctx.GetGolomb();
    const bool map = ctx.ComputeMap(errorValue, k);
    const int32_t EMErrval = 2 * std::abs(errorValue) - ctx.nRItype_ - static_cast<int32_t>(map);

    ASSERT(errorValue == ctx.ComputeErrVal(EMErrval + ctx.nRItype_, k));
    EncodeMappedValue(k, EMErrval, traits.LIMIT - J[RUNindex_] - 1);
    ctx.UpdateVariables(errorValue, EMErrval);
}


template<typename Traits, typename Strategy>
Triplet<typename Traits::SAMPLE> JlsCodec<Traits, Strategy>::DecodeRIPixel(Triplet<SAMPLE> Ra, Triplet<SAMPLE> Rb)
{
    const int32_t errorValue1 = DecodeRIError(contextRunmode_[0]);
    const int32_t errorValue2 = DecodeRIError(contextRunmode_[0]);
    const int32_t errorValue3 = DecodeRIError(contextRunmode_[0]);

    return Triplet<SAMPLE>(traits.ComputeReconstructedSample(Rb.v1, errorValue1 * Sign(Rb.v1 - Ra.v1)),
                           traits.ComputeReconstructedSample(Rb.v2, errorValue2 * Sign(Rb.v2 - Ra.v2)),
                           traits.ComputeReconstructedSample(Rb.v3, errorValue3 * Sign(Rb.v3 - Ra.v3)));
}


template<typename Traits, typename Strategy>
Triplet<typename Traits::SAMPLE> JlsCodec<Traits, Strategy>::EncodeRIPixel(Triplet<SAMPLE> x, Triplet<SAMPLE> Ra, Triplet<SAMPLE> Rb)
{
    const int32_t errorValue1 = traits.ComputeErrVal(Sign(Rb.v1 - Ra.v1) * (x.v1 - Rb.v1));
    EncodeRIError(contextRunmode_[0], errorValue1);

    const int32_t errorValue2 = traits.ComputeErrVal(Sign(Rb.v2 - Ra.v2) * (x.v2 - Rb.v2));
    EncodeRIError(contextRunmode_[0], errorValue2);

    const int32_t errorValue3 = traits.ComputeErrVal(Sign(Rb.v3 - Ra.v3) * (x.v3 - Rb.v3));
    EncodeRIError(contextRunmode_[0], errorValue3);

    return Triplet<SAMPLE>(traits.ComputeReconstructedSample(Rb.v1, errorValue1 * Sign(Rb.v1 - Ra.v1)),
                           traits.ComputeReconstructedSample(Rb.v2, errorValue2 * Sign(Rb.v2 - Ra.v2)),
                           traits.ComputeReconstructedSample(Rb.v3, errorValue3 * Sign(Rb.v3 - Ra.v3)));
}

template<typename Traits, typename Strategy>
Quad<typename Traits::SAMPLE> JlsCodec<Traits, Strategy>::DecodeRIPixel(Quad<SAMPLE> Ra, Quad<SAMPLE> Rb)
{
    const int32_t errorValue1 = DecodeRIError(contextRunmode_[0]);
    const int32_t errorValue2 = DecodeRIError(contextRunmode_[0]);
    const int32_t errorValue3 = DecodeRIError(contextRunmode_[0]);
    const int32_t errorValue4 = DecodeRIError(contextRunmode_[0]);

    return Quad<SAMPLE>(Triplet<SAMPLE>(traits.ComputeReconstructedSample(Rb.v1, errorValue1 * Sign(Rb.v1 - Ra.v1)),
                                        traits.ComputeReconstructedSample(Rb.v2, errorValue2 * Sign(Rb.v2 - Ra.v2)),
                                        traits.ComputeReconstructedSample(Rb.v3, errorValue3 * Sign(Rb.v3 - Ra.v3))),
                        traits.ComputeReconstructedSample(Rb.v4, errorValue4 * Sign(Rb.v4 - Ra.v4)));
}


template<typename Traits, typename Strategy>
Quad<typename Traits::SAMPLE> JlsCodec<Traits, Strategy>::EncodeRIPixel(Quad<SAMPLE> x, Quad<SAMPLE> Ra, Quad<SAMPLE> Rb)
{
    const int32_t errorValue1 = traits.ComputeErrVal(Sign(Rb.v1 - Ra.v1) * (x.v1 - Rb.v1));
    EncodeRIError(contextRunmode_[0], errorValue1);

    const int32_t errorValue2 = traits.ComputeErrVal(Sign(Rb.v2 - Ra.v2) * (x.v2 - Rb.v2));
    EncodeRIError(contextRunmode_[0], errorValue2);

    const int32_t errorValue3 = traits.ComputeErrVal(Sign(Rb.v3 - Ra.v3) * (x.v3 - Rb.v3));
    EncodeRIError(contextRunmode_[0], errorValue3);

    const int32_t errorValue4 = traits.ComputeErrVal(Sign(Rb.v4 - Ra.v4) * (x.v4 - Rb.v4));
    EncodeRIError(contextRunmode_[0], errorValue4);

    return Quad<SAMPLE>(Triplet<SAMPLE>(traits.ComputeReconstructedSample(Rb.v1, errorValue1 * Sign(Rb.v1 - Ra.v1)),
                                        traits.ComputeReconstructedSample(Rb.v2, errorValue2 * Sign(Rb.v2 - Ra.v2)),
                                        traits.ComputeReconstructedSample(Rb.v3, errorValue3 * Sign(Rb.v3 - Ra.v3))),
                        traits.ComputeReconstructedSample(Rb.v4, errorValue4 * Sign(Rb.v4 - Ra.v4)));
}


template<typename Traits, typename Strategy>
typename Traits::SAMPLE JlsCodec<Traits, Strategy>::DecodeRIPixel(int32_t Ra, int32_t Rb)
{
    if (std::abs(Ra - Rb) <= traits.NEAR)
    {
        const int32_t ErrVal = DecodeRIError(contextRunmode_[1]);
        return static_cast<SAMPLE>(traits.ComputeReconstructedSample(Ra, ErrVal));
    }

    const int32_t ErrVal = DecodeRIError(contextRunmode_[0]);
    return static_cast<SAMPLE>(traits.ComputeReconstructedSample(Rb, ErrVal * Sign(Rb - Ra)));
}


template<typename Traits, typename Strategy>
typename Traits::SAMPLE JlsCodec<Traits, Strategy>::EncodeRIPixel(int32_t x, int32_t Ra, int32_t Rb)
{
    if (std::abs(Ra - Rb) <= traits.NEAR)
    {
        const int32_t ErrVal = traits.ComputeErrVal(x - Ra);
        EncodeRIError(contextRunmode_[1], ErrVal);
        return static_cast<SAMPLE>(traits.ComputeReconstructedSample(Ra, ErrVal));
    }

    const int32_t ErrVal = traits.ComputeErrVal((x - Rb) * Sign(Rb - Ra));
    EncodeRIError(contextRunmode_[0], ErrVal);
    return static_cast<SAMPLE>(traits.ComputeReconstructedSample(Rb, ErrVal * Sign(Rb - Ra)));
}


// RunMode: Functions that handle run-length encoding

template<typename Traits, typename Strategy>
void JlsCodec<Traits, Strategy>::EncodeRunPixels(int32_t runLength, bool endOfLine)
{
    while (runLength >= static_cast<int32_t>(1 << J[RUNindex_]))
    {
        Strategy::AppendOnesToBitStream(1);
        runLength = runLength - static_cast<int32_t>(1 << J[RUNindex_]);
        IncrementRunIndex();
    }

    if (endOfLine)
    {
        if (runLength != 0)
        {
            Strategy::AppendOnesToBitStream(1);
        }
    }
    else
    {
        Strategy::AppendToBitStream(runLength, J[RUNindex_] + 1); // leading 0 + actual remaining length
    }
}


template<typename Traits, typename Strategy>
int32_t JlsCodec<Traits, Strategy>::DecodeRunPixels(PIXEL Ra, PIXEL* startPos, int32_t cpixelMac)
{
    int32_t index = 0;
    while (Strategy::ReadBit())
    {
        const int count = std::min(1 << J[RUNindex_], int(cpixelMac - index));
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
        index += (J[RUNindex_] > 0) ? Strategy::ReadValue(J[RUNindex_]) : 0;
    }

    if (index > cpixelMac)
        throw jpegls_error{jpegls_errc::invalid_encoded_data};

    for (int32_t i = 0; i < index; ++i)
    {
        startPos[i] = Ra;
    }

    return index;
}

template<typename Traits, typename Strategy>
int32_t JlsCodec<Traits, Strategy>::DoRunMode(int32_t index, EncoderStrategy*)
{
    const int32_t ctypeRem = width_ - index;
    PIXEL* ptypeCurX = currentLine_ + index;
    const PIXEL* ptypePrevX = previousLine_ + index;

    const PIXEL Ra = ptypeCurX[-1];

    int32_t runLength = 0;

    while (traits.IsNear(ptypeCurX[runLength], Ra))
    {
        ptypeCurX[runLength] = Ra;
        runLength++;

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
int32_t JlsCodec<Traits, Strategy>::DoRunMode(int32_t startIndex, DecoderStrategy*)
{
    const PIXEL Ra = currentLine_[startIndex - 1];

    const int32_t runLength = DecodeRunPixels(Ra, currentLine_ + startIndex, width_ - startIndex);
    const int32_t endIndex = startIndex + runLength;

    if (endIndex == width_)
        return endIndex - startIndex;

    // run interruption
    const PIXEL Rb = previousLine_[endIndex];
    currentLine_[endIndex] = DecodeRIPixel(Ra, Rb);
    DecrementRunIndex();
    return endIndex - startIndex + 1;
}


/// <summary>Encodes/Decodes a scan line of samples</summary>
template<typename Traits, typename Strategy>
void JlsCodec<Traits, Strategy>::DoLine(SAMPLE*)
{
    int32_t index = 0;
    int32_t Rb = previousLine_[index - 1];
    int32_t Rd = previousLine_[index];

    while (index < width_)
    {
        const int32_t Ra = currentLine_[index - 1];
        const int32_t Rc = Rb;
        Rb = Rd;
        Rd = previousLine_[index + 1];

        const int32_t Qs = ComputeContextID(QuantizeGradient(Rd - Rb), QuantizeGradient(Rb - Rc), QuantizeGradient(Rc - Ra));

        if (Qs != 0)
        {
            currentLine_[index] = DoRegular(Qs, currentLine_[index], GetPredictedValue(Ra, Rb, Rc), static_cast<Strategy*>(nullptr));
            index++;
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
void JlsCodec<Traits, Strategy>::DoLine(Triplet<SAMPLE>*)
{
    int32_t index = 0;
    while (index < width_)
    {
        const Triplet<SAMPLE> Ra = currentLine_[index - 1];
        const Triplet<SAMPLE> Rc = previousLine_[index - 1];
        const Triplet<SAMPLE> Rb = previousLine_[index];
        const Triplet<SAMPLE> Rd = previousLine_[index + 1];

        const int32_t Qs1 = ComputeContextID(QuantizeGradient(Rd.v1 - Rb.v1), QuantizeGradient(Rb.v1 - Rc.v1), QuantizeGradient(Rc.v1 - Ra.v1));
        const int32_t Qs2 = ComputeContextID(QuantizeGradient(Rd.v2 - Rb.v2), QuantizeGradient(Rb.v2 - Rc.v2), QuantizeGradient(Rc.v2 - Ra.v2));
        const int32_t Qs3 = ComputeContextID(QuantizeGradient(Rd.v3 - Rb.v3), QuantizeGradient(Rb.v3 - Rc.v3), QuantizeGradient(Rc.v3 - Ra.v3));

        if (Qs1 == 0 && Qs2 == 0 && Qs3 == 0)
        {
            index += DoRunMode(index, static_cast<Strategy*>(nullptr));
        }
        else
        {
            Triplet<SAMPLE> Rx;
            Rx.v1 = DoRegular(Qs1, currentLine_[index].v1, GetPredictedValue(Ra.v1, Rb.v1, Rc.v1), static_cast<Strategy*>(nullptr));
            Rx.v2 = DoRegular(Qs2, currentLine_[index].v2, GetPredictedValue(Ra.v2, Rb.v2, Rc.v2), static_cast<Strategy*>(nullptr));
            Rx.v3 = DoRegular(Qs3, currentLine_[index].v3, GetPredictedValue(Ra.v3, Rb.v3, Rc.v3), static_cast<Strategy*>(nullptr));
            currentLine_[index] = Rx;
            index++;
        }
    }
}


/// <summary>Encodes/Decodes a scan line of quads in ILV_SAMPLE mode</summary>
template<typename Traits, typename Strategy>
void JlsCodec<Traits, Strategy>::DoLine(Quad<SAMPLE>*)
{
    int32_t index = 0;
    while (index < width_)
    {
        const Quad<SAMPLE> Ra = currentLine_[index - 1];
        const Quad<SAMPLE> Rc = previousLine_[index - 1];
        const Quad<SAMPLE> Rb = previousLine_[index];
        const Quad<SAMPLE> Rd = previousLine_[index + 1];

        const int32_t Qs1 = ComputeContextID(QuantizeGradient(Rd.v1 - Rb.v1), QuantizeGradient(Rb.v1 - Rc.v1), QuantizeGradient(Rc.v1 - Ra.v1));
        const int32_t Qs2 = ComputeContextID(QuantizeGradient(Rd.v2 - Rb.v2), QuantizeGradient(Rb.v2 - Rc.v2), QuantizeGradient(Rc.v2 - Ra.v2));
        const int32_t Qs3 = ComputeContextID(QuantizeGradient(Rd.v3 - Rb.v3), QuantizeGradient(Rb.v3 - Rc.v3), QuantizeGradient(Rc.v3 - Ra.v3));
        const int32_t Qs4 = ComputeContextID(QuantizeGradient(Rd.v4 - Rb.v4), QuantizeGradient(Rb.v4 - Rc.v4), QuantizeGradient(Rc.v4 - Ra.v4));

        if (Qs1 == 0 && Qs2 == 0 && Qs3 == 0 && Qs4 == 0)
        {
            index += DoRunMode(index, static_cast<Strategy*>(nullptr));
        }
        else
        {
            Quad<SAMPLE> Rx;
            Rx.v1 = DoRegular(Qs1, currentLine_[index].v1, GetPredictedValue(Ra.v1, Rb.v1, Rc.v1), static_cast<Strategy*>(nullptr));
            Rx.v2 = DoRegular(Qs2, currentLine_[index].v2, GetPredictedValue(Ra.v2, Rb.v2, Rc.v2), static_cast<Strategy*>(nullptr));
            Rx.v3 = DoRegular(Qs3, currentLine_[index].v3, GetPredictedValue(Ra.v3, Rb.v3, Rc.v3), static_cast<Strategy*>(nullptr));
            Rx.v4 = DoRegular(Qs4, currentLine_[index].v4, GetPredictedValue(Ra.v4, Rb.v4, Rc.v4), static_cast<Strategy*>(nullptr));
            currentLine_[index] = Rx;
            index++;
        }
    }
}


// DoScan: Encodes or decodes a scan.
// In ILV_SAMPLE mode, multiple components are handled in DoLine
// In ILV_LINE mode, a call do DoLine is made for every component
// In ILV_NONE mode, DoScan is called for each component

template<typename Traits, typename Strategy>
void JlsCodec<Traits, Strategy>::DoScan()
{
    const int32_t pixelStride = width_ + 4;
    const int components = Info().interleaveMode == interleave_mode::line ? Info().components : 1;

    std::vector<PIXEL> vectmp(static_cast<size_t>(2) * components * pixelStride);
    std::vector<int32_t> rgRUNindex(components);

    for (int32_t line = 0; line < Info().height; ++line)
    {
        previousLine_ = &vectmp[1];
        currentLine_ = &vectmp[1 + static_cast<size_t>(components) * pixelStride];
        if ((line & 1) == 1)
        {
            std::swap(previousLine_, currentLine_);
        }

        Strategy::OnLineBegin(width_, currentLine_, pixelStride);

        for (int component = 0; component < components; ++component)
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

        if (rect_.Y <= line && line < rect_.Y + rect_.Height)
        {
            Strategy::OnLineEnd(rect_.Width, currentLine_ + rect_.X - (static_cast<size_t>(components) * pixelStride), pixelStride);
        }
    }

    Strategy::EndScan();
}


// Factory function for ProcessLine objects to copy/transform un encoded pixels to/from our scan line buffers.
template<typename Traits, typename Strategy>
std::unique_ptr<ProcessLine> JlsCodec<Traits, Strategy>::CreateProcess(ByteStreamInfo info)
{
    if (!IsInterleaved())
    {
        return info.rawData ?
            std::unique_ptr<ProcessLine>(std::make_unique<PostProcessSingleComponent>(info.rawData, Info().stride, sizeof(typename Traits::PIXEL))) :
            std::unique_ptr<ProcessLine>(std::make_unique<PostProcessSingleStream>(info.rawStream, Info().stride, sizeof(typename Traits::PIXEL)));
    }

    if (Info().colorTransformation == color_transformation::none)
        return std::make_unique<ProcessTransformed<TransformNone<typename Traits::SAMPLE>>>(info, Info(), TransformNone<SAMPLE>());

    if (Info().bitsPerSample == sizeof(SAMPLE) * 8)
    {
        switch (Info().colorTransformation)
        {
        case color_transformation::hp1:
            return std::make_unique<ProcessTransformed<TransformHp1<SAMPLE>>>(info, Info(), TransformHp1<SAMPLE>());
        case color_transformation::hp2:
            return std::make_unique<ProcessTransformed<TransformHp2<SAMPLE>>>(info, Info(), TransformHp2<SAMPLE>());
        case color_transformation::hp3:
            return std::make_unique<ProcessTransformed<TransformHp3<SAMPLE>>>(info, Info(), TransformHp3<SAMPLE>());
        default:
            throw jpegls_error{jpegls_errc::color_transform_not_supported};
        }
    }

    if (Info().bitsPerSample > 8)
    {
        const int shift = 16 - Info().bitsPerSample;
        switch (Info().colorTransformation)
        {
        case color_transformation::hp1:
            return std::make_unique<ProcessTransformed<TransformShifted<TransformHp1<uint16_t>>>>(info, Info(), TransformShifted<TransformHp1<uint16_t>>(shift));
        case color_transformation::hp2:
            return std::make_unique<ProcessTransformed<TransformShifted<TransformHp2<uint16_t>>>>(info, Info(), TransformShifted<TransformHp2<uint16_t>>(shift));
        case color_transformation::hp3:
            return std::make_unique<ProcessTransformed<TransformShifted<TransformHp3<uint16_t>>>>(info, Info(), TransformShifted<TransformHp3<uint16_t>>(shift));
        default:
            throw jpegls_error{jpegls_errc::color_transform_not_supported};
        }
    }

    throw jpegls_error{jpegls_errc::bit_depth_for_transform_not_supported};
}


// Setup codec for encoding and calls DoScan
MSVC_WARNING_SUPPRESS(26433) // C.128: Virtual functions should specify exactly one of virtual, override, or final
template<typename Traits, typename Strategy>
size_t JlsCodec<Traits, Strategy>::EncodeScan(std::unique_ptr<ProcessLine> processLine, ByteStreamInfo& compressedData)
{
    Strategy::processLine_ = std::move(processLine);

    Strategy::Init(compressedData);
    DoScan();

    return Strategy::GetLength();
}


// Setup codec for decoding and calls DoScan
template<typename Traits, typename Strategy>
void JlsCodec<Traits, Strategy>::DecodeScan(std::unique_ptr<ProcessLine> processLine, const JlsRect& rect, ByteStreamInfo& compressedData)
{
    Strategy::processLine_ = std::move(processLine);

    const uint8_t* compressedBytes = compressedData.rawData;
    rect_ = rect;

    Strategy::Init(compressedData);
    DoScan();
    SkipBytes(compressedData, Strategy::GetCurBytePos() - compressedBytes);
}
MSVC_WARNING_UNSUPPRESS()

// Initialize the codec data structures. Depends on JPEG-LS parameters like Threshold1-Threshold3.
template<typename Traits, typename Strategy>
void JlsCodec<Traits, Strategy>::InitParams(int32_t t1, int32_t t2, int32_t t3, int32_t nReset)
{
    T1 = t1;
    T2 = t2;
    T3 = t3;

    InitQuantizationLUT();

    const JlsContext contextInitValue(std::max(2, (traits.RANGE + 32) / 64));
    for (auto& context : contexts_)
    {
        context = contextInitValue;
    }

    contextRunmode_[0] = CContextRunMode(std::max(2, (traits.RANGE + 32) / 64), 0, nReset);
    contextRunmode_[1] = CContextRunMode(std::max(2, (traits.RANGE + 32) / 64), 1, nReset);
    RUNindex_ = 0;
}

} // namespace charls
