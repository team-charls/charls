// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "../src/quantization_lut.hpp"
#include "../src/jpegls_algorithm.hpp"
#include "../src/jpegls_preset_coding_parameters.hpp"

#include <vector>

namespace charls::test {

namespace {

// Runtime verification: every LUT entry matches the on-the-fly computation.
void verify(const std::vector<int8_t>& lut, const int32_t bit_count)
{
    const auto preset{compute_default(calculate_maximum_bit_sample_value(bit_count), 0)};
    const int32_t range{preset.maximum_sample_value + 1};

     ASSERT_TRUE(lut.size() == static_cast<size_t>(range) * 2);
     for (size_t i{}; i != lut.size(); ++i)
    {
         ASSERT_TRUE(lut[i] == quantize_gradient_org(static_cast<int32_t>(i) - range, preset.threshold1,
                                                         preset.threshold2, preset.threshold3));
     }
}

} // namespace

TEST(quantization_lut_test, lossless_8)
{
    verify(quantization_lut_lossless_8(), 8);
}

TEST(quantization_lut_test, lossless_10)
{
    verify(quantization_lut_lossless_10(), 10);
}

TEST(quantization_lut_test, lossless_12)
{
    verify(quantization_lut_lossless_12(), 12);
}

TEST(quantization_lut_test, lossless_16)
{
    verify(quantization_lut_lossless_16(), 16);
}

} // namespace charls::test
