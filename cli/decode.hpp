// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <filesystem>

namespace charls::cli {

void decode_to_pnm(const std::filesystem::path& filename_input, const std::filesystem::path& filename_output);

}
