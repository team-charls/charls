// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>

namespace charls {

enum class jpegls_preset_parameters_type : uint8_t
{
    preset_coding_parameters = 0x1,                     // JPEG-LS Baseline (ISO/IEC 14495-1): Preset coding parameters.
    mapping_table_specification = 0x2,                  // JPEG-LS Baseline (ISO/IEC 14495-1): Mapping table specification.
    mapping_table_continuation = 0x3,                   // JPEG-LS Baseline (ISO/IEC 14495-1): Mapping table continuation.
    extended_width_and_height = 0x4,                    // JPEG-LS Baseline (ISO/IEC 14495-1): X and Y parameters greater than 16 bits are defined.
    coding_method_specification = 0x5,                  // JPEG-LS Extended (ISO/IEC 14495-2): Coding method specification.
    near_lossless_error_re_specification = 0x6,         // JPEG-LS Extended (ISO/IEC 14495-2): NEAR value re-specification.
    visually_oriented_quantization_specification = 0x7, // JPEG-LS Extended (ISO/IEC 14495-2): Visually oriented quantization specification.
    extended_prediction_specification = 0x8,            // JPEG-LS Extended (ISO/IEC 14495-2): Extended prediction specification.
    start_of_fixed_length_coding = 0x9,                 // JPEG-LS Extended (ISO/IEC 14495-2): Specification of the start of fixed length coding.
    end_of_fixed_length_coding = 0xA,                   // JPEG-LS Extended (ISO/IEC 14495-2): Specification of the end of fixed length coding.
    extended_preset_coding_parameters = 0xC,            // JPEG-LS Extended (ISO/IEC 14495-2): JPEG-LS preset coding parameters.
    inverse_color_transform_specification = 0xD         // JPEG-LS Extended (ISO/IEC 14495-2): inverse color transform specification.
};

} // namespace charls
