// SPDX-FileCopyrightText: © 2016 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>
#include <filesystem>

namespace charls::cli {

void benchmark_decode(const std::filesystem::path& filename, std::uint32_t loop_count);
void benchmark_encode(const std::filesystem::path& filename, std::uint32_t loop_count);

} // namespace charls::cli
