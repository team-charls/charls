// SPDX-FileCopyrightText: © 2023 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "quantization_lut.hpp"

#include <charls/public_types.h>

#include "jpegls_algorithm.hpp"
#include "jpegls_preset_coding_parameters.hpp"

namespace charls {

using std::vector;

namespace {

vector<int8_t> create_quantize_lut_lossless(const int32_t bit_count)
{
    const jpegls_pc_parameters preset{compute_default(calculate_maximum_sample_value(bit_count), 0)};
    const int32_t range{preset.maximum_sample_value + 1};

    vector<int8_t> lut(static_cast<size_t>(range) * 2);
    for (size_t i{}; i != lut.size(); ++i)
    {
        lut[i] =
            quantize_gradient_org(static_cast<int32_t>(i) - range, preset.threshold1, preset.threshold2, preset.threshold3);
    }

    return lut;
}

} // namespace

// Lookup tables: sample differences to bin indexes.
// ReSharper disable CppTemplateArgumentsCanBeDeduced
// NOLINTNEXTLINE(clang-diagnostic-global-constructors)
const vector<int8_t> quantization_lut_lossless_8{create_quantize_lut_lossless(8)};

// NOLINTNEXTLINE(clang-diagnostic-global-constructors)
const vector<int8_t> quantization_lut_lossless_10{create_quantize_lut_lossless(10)};

// NOLINTNEXTLINE(clang-diagnostic-global-constructors)
const vector<int8_t> quantization_lut_lossless_12{create_quantize_lut_lossless(12)};

// NOLINTNEXTLINE(clang-diagnostic-global-constructors)
const vector<int8_t> quantization_lut_lossless_16{create_quantize_lut_lossless(16)};
// ReSharper restore CppTemplateArgumentsCanBeDeduced

} // namespace charls
