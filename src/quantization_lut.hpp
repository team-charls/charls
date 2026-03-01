// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>
#include <vector>

namespace charls {

const std::vector<int8_t>& quantization_lut_lossless_8();
const std::vector<int8_t>& quantization_lut_lossless_10();
const std::vector<int8_t>& quantization_lut_lossless_12();
const std::vector<int8_t>& quantization_lut_lossless_16();

} // namespace charls
