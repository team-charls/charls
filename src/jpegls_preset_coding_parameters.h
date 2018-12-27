// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

#include <charls/public_types.h>
#include "constants.h"

#include <algorithm>

namespace charls {

/// <summary>Clamping function as defined by ISO/IEC 14495-1, Figure C.3</summary>
inline int32_t clamp(int32_t i, int32_t j, int32_t maximumSampleValue) noexcept
{
    if (i > maximumSampleValue || i < j)
        return j;

    return i;
}

inline JpegLSPresetCodingParameters ComputeDefault(const int32_t maximumSampleValue, const int32_t allowedLossyError) noexcept
{
    const int32_t factor = (std::min(maximumSampleValue, 4095) + 128) / 256;
    const int threshold1 = clamp(factor * (DefaultThreshold1 - 2) + 2 + 3 * allowedLossyError, allowedLossyError + 1, maximumSampleValue);
    const int threshold2 = clamp(factor * (DefaultThreshold2 - 3) + 3 + 5 * allowedLossyError, threshold1, maximumSampleValue); //-V537

    return {
        maximumSampleValue,
        threshold1,
        threshold2,
        clamp(factor * (DefaultThreshold3 - 4) + 4 + 7 * allowedLossyError, threshold2, maximumSampleValue),
        DefaultResetValue};
}

inline bool IsDefault(const JpegLSPresetCodingParameters& custom) noexcept
{
    if (custom.MaximumSampleValue != 0)
        return false;

    if (custom.Threshold1 != 0)
        return false;

    if (custom.Threshold2 != 0)
        return false;

    if (custom.Threshold3 != 0)
        return false;

    if (custom.ResetValue != 0)
        return false;

    return true;
}

} // namespace charls
