// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

namespace charls
{

// Default threshold values for JPEG-LS statistical modeling as defined in ISO/IEC 14495-1, Table C.3
// for the case MAXVAL = 255 and NEAR = 0.
// Can be overridden at compression time, however this is rarely done.
constexpr int DefaultThreshold1 = 3;  // BASIC_T1
constexpr int DefaultThreshold2 = 7; // BASIC_T2
constexpr int DefaultThreshold3 = 21; // BASIC_T3

constexpr int DefaultResetValue = 64; // Default RESET value as defined in  ISO/IEC 14495-1, table C.2

constexpr int MaximumComponentCount = 255;
constexpr int MinimumBitsPerSample = 2;
constexpr int MaximumBitsPerSample = 16;

} // namespace charls
