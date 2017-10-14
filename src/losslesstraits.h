//
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use.
//

#ifndef CHARLS_LOSSLESSTRAITS
#define CHARLS_LOSSLESSTRAITS

#include "constants.h"

// Optimized trait classes for lossless compression of 8 bit color and 8/16 bit monochrome images.
// This class assumes MaximumSampleValue correspond to a whole number of bits, and no custom ResetValue is set when encoding.
// The point of this is to have the most optimized code for the most common and most demanding scenario.
template<typename sample, int32_t bitsperpixel>
struct LosslessTraitsImpl
{
    using SAMPLE = sample;

    enum
    {
        NEAR  = 0,
        bpp   = bitsperpixel,
        qbpp  = bitsperpixel,
        RANGE = (1 << bpp),
        MAXVAL= (1 << bpp) - 1,
        LIMIT = 2 * (bitsperpixel + std::max(8, bitsperpixel)),
        RESET = DefaultResetValue
    };

    static force_inline int32_t ComputeErrVal(int32_t d)
    {
        return ModuloRange(d);
    }

    static force_inline bool IsNear(int32_t lhs, int32_t rhs)
    {
        return lhs == rhs;
    }

// The following optimization is implementation-dependent (works on x86 and ARM, see charlstest).
#if defined(__clang__)
     __attribute__((no_sanitize("shift")))
#endif
    static force_inline int32_t ModuloRange(int32_t errorValue)
    {
        return static_cast<int32_t>(errorValue << (INT32_BITCOUNT - bpp)) >> (INT32_BITCOUNT - bpp);
    }

    static force_inline SAMPLE ComputeReconstructedSample(int32_t Px, int32_t ErrVal)
    {
        return static_cast<SAMPLE>(MAXVAL & (Px + ErrVal));
    }

    static force_inline int32_t CorrectPrediction(int32_t Pxc)
    {
        if ((Pxc & MAXVAL) == Pxc)
            return Pxc;

        return (~(Pxc >> (INT32_BITCOUNT-1))) & MAXVAL;
    }
};


template<typename T, int32_t bpp>
struct LosslessTraits : LosslessTraitsImpl<T, bpp>
{
    using PIXEL = T;
};


template<>
struct LosslessTraits<uint8_t, 8> : LosslessTraitsImpl<uint8_t, 8>
{
    using PIXEL = SAMPLE;

    static force_inline signed char ModRange(int32_t Errval)
    {
        return static_cast<signed char>(Errval);
    }

    static force_inline int32_t ComputeErrVal(int32_t d)
    {
        return static_cast<signed char>(d);
    }

    static force_inline uint8_t ComputeReconstructedSample(int32_t Px, int32_t ErrVal)
    {
        return static_cast<uint8_t>(Px + ErrVal);
    }
};


template<>
struct LosslessTraits<uint16_t, 16> : LosslessTraitsImpl<uint16_t, 16>
{
    using PIXEL = SAMPLE;

    static force_inline short ModRange(int32_t Errval)
    {
        return static_cast<short>(Errval);
    }

    static force_inline int32_t ComputeErrVal(int32_t d)
    {
        return static_cast<short>(d);
    }

    static force_inline SAMPLE ComputeReconstructedSample(int32_t Px, int32_t ErrVal)
    {
        return static_cast<SAMPLE>(Px + ErrVal);
    }
};


template<typename T, int32_t bpp>
struct LosslessTraits<Triplet<T>, bpp> : LosslessTraitsImpl<T, bpp>
{
    using PIXEL = Triplet<T>;

    static force_inline bool IsNear(int32_t lhs, int32_t rhs)
    {
        return lhs == rhs;
    }

    static force_inline bool IsNear(PIXEL lhs, PIXEL rhs)
    {
        return lhs == rhs;
    }

    static force_inline T ComputeReconstructedSample(int32_t Px, int32_t ErrVal)
    {
        return static_cast<T>(Px + ErrVal);
    }
};

#endif
