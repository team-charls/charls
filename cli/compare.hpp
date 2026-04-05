// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <filesystem>

namespace charls::cli {

[[nodiscard]]
bool compare_netpbm(const std::filesystem::path& filename1, const std::filesystem::path& filename2);

}
