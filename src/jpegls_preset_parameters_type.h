// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>

namespace charls {

enum class jpegls_preset_parameters_type : uint8_t
{
    /// <summary>
    /// JPEG-LS Baseline (ISO/IEC 14495-1): Preset coding parameters (defined in C.2.4.1.1).
    /// </summary>
    preset_coding_parameters = 0x1,

    /// <summary>
    /// JPEG-LS Baseline (ISO/IEC 14495-1): Mapping table specification (defined in C.2.4.1.2).
    /// </summary>
    mapping_table_specification = 0x2,

    /// <summary>
    /// JPEG-LS Baseline (ISO/IEC 14495-1): Mapping table continuation (defined in C.2.4.1.3).
    /// </summary>
    mapping_table_continuation = 0x3,

    /// <summary>
    /// JPEG-LS Baseline (ISO/IEC 14495-1): X and Y parameters are defined (defined in C.2.4.1.4).
    /// </summary>
    oversize_image_dimension = 0x4,
};

} // namespace charls
