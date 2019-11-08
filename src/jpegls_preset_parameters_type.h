// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>

namespace charls {

enum class JpegLSPresetParametersType : uint8_t
{
    PresetCodingParameters = 0x1,                    // JPEG-LS Baseline (ISO/IEC 14495-1): Preset coding parameters.
    MappingTableSpecification = 0x2,                 // JPEG-LS Baseline (ISO/IEC 14495-1): Mapping table specification.
    MappingTableContinuation = 0x3,                  // JPEG-LS Baseline (ISO/IEC 14495-1): Mapping table continuation.
    ExtendedWidthAndHeight = 0x4,                    // JPEG-LS Baseline (ISO/IEC 14495-1): X and Y parameters greater than 16 bits are defined.
    CodingMethodSpecification = 0x5,                 // JPEG-LS Extended (ISO/IEC 14495-2): Coding method specification.
    NearLosslessErrorReSpecification = 0x6,          // JPEG-LS Extended (ISO/IEC 14495-2): NEAR value re-specification.
    VisuallyOrientedQuantizationSpecification = 0x7, // JPEG-LS Extended (ISO/IEC 14495-2): Visually oriented quantization specification.
    ExtendedPredictionSpecification = 0x8,           // JPEG-LS Extended (ISO/IEC 14495-2): Extended prediction specification.
    StartOfFixedLengthCoding = 0x9,                  // JPEG-LS Extended (ISO/IEC 14495-2): Specification of the start of fixed length coding.
    EndOfFixedLengthCoding = 0xA,                    // JPEG-LS Extended (ISO/IEC 14495-2): Specification of the end of fixed length coding.
    ExtendedPresetCodingParameters = 0xC,            // JPEG-LS Extended (ISO/IEC 14495-2): JPEG-LS preset coding parameters.
    InverseColorTransformSpecification = 0xD         // JPEG-LS Extended (ISO/IEC 14495-2): Inverse color transform specification.
};

}
