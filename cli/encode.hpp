// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>
#include <filesystem>

namespace charls::cli {

constexpr int32_t default_interleave_mode{-1};

void encode_netpbm(const std::filesystem::path& filename_input, const std::filesystem::path& filename_output,
                   int32_t interleave_mode, int32_t near_lossless, int32_t color_transformation);

} // namespace charls::cli
