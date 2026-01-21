// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "version.h"

namespace charls {

inline constexpr int32_t version_major{CHARLS_VERSION_MAJOR};
inline constexpr int32_t version_minor{CHARLS_VERSION_MINOR};
inline constexpr int32_t version_patch{CHARLS_VERSION_PATCH};

/// <summary>
/// Returns the version of CharLS in the semantic versioning format "major.minor.patch" or "major.minor.patch-pre_release"
/// </summary>
[[nodiscard]]
inline const char* version_string() noexcept
{
    return charls_get_version_string();
}

struct version final
{
    std::int32_t major;
    std::int32_t minor;
    std::int32_t patch;
};

/// <summary>
/// Returns the version of CharLS in its numerical format.
/// This method retrieves the numbers from the library implementation, 
/// which may be different from the constexpr values if a shared library or DLL is used.
/// </summary>
[[nodiscard]]
inline version version_number() noexcept
{
    version version;
    charls_get_version_number(&version.major, &version.minor, &version.patch);
    return version;
}

} // namespace charls
