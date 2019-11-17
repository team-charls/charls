// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <algorithm>

namespace charls {

// Default threshold values for JPEG-LS statistical modeling as defined in ISO/IEC 14495-1, Table C.3
// for the case MAXVAL = 255 and NEAR = 0.
// Can be overridden at compression time, however this is rarely done.
constexpr int DefaultThreshold1 = 3;  // BASIC_T1
constexpr int DefaultThreshold2 = 7;  // BASIC_T2
constexpr int DefaultThreshold3 = 21; // BASIC_T3

constexpr int DefaultResetValue = 64; // Default RESET value as defined in ISO/IEC 14495-1, table C.2

constexpr int maximum_width = 65535;
constexpr int maximum_height = 65535;
constexpr int MaximumComponentCount = 255;
constexpr int MinimumBitsPerSample = 2;
constexpr int MaximumBitsPerSample = 16;
constexpr int maximum_near_lossless = 255;

constexpr int MaximumNearLossless(const int maximumSampleValue) noexcept
{
    return std::min(255, maximumSampleValue / 2); // As defined by ISO/IEC 14495-1, C.2.3
}

// ISO/IEC 14495-1, section 4.8.1 defines the SPIFF version numbers to be used for the SPIFF header in combination with JPEG-LS.
constexpr uint8_t spiff_major_revision_number = 2;
constexpr uint8_t spiff_minor_revision_number = 0;

constexpr uint8_t spiff_end_of_directory_entry_type = 1;

// The size of a SPIFF header when serialized to a JPEG byte stream.
constexpr size_t spiff_header_size_in_bytes = 34;


} // namespace charls
