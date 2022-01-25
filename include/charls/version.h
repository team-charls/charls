// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "annotations.h"
#include "api_abi.h"

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif


#define CHARLS_VERSION_MAJOR 2
#define CHARLS_VERSION_MINOR 3
#define CHARLS_VERSION_PATCH 1

/// <summary>
/// Returns the version of CharLS in the semver format "major.minor.patch" or "major.minor.patch-pre_release"
/// </summary>
CHARLS_CHECK_RETURN CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT const char*
    CHARLS_API_CALLING_CONVENTION charls_get_version_string(CHARLS_C_VOID);

/// <summary>
/// Returns the version of CharLS in its numerical format.
/// </summary>
/// <param name="major">Reference to the major number, may be NULL/nullptr when this info is not needed.</param>
/// <param name="minor">Reference to the minor number, may be NULL/nullptr when this info is not needed.</param>
/// <param name="patch">Reference to the patch number, may be NULL/nullptr when this info is not needed.</param>
CHARLS_API_IMPORT_EXPORT void CHARLS_API_CALLING_CONVENTION charls_get_version_number(CHARLS_OUT_OPT int32_t* major,
                                                                                      CHARLS_OUT_OPT int32_t* minor,
                                                                                      CHARLS_OUT_OPT int32_t* patch);

#ifdef __cplusplus
}

namespace charls {

constexpr int32_t version_major{CHARLS_VERSION_MAJOR};
constexpr int32_t version_minor{CHARLS_VERSION_MINOR};
constexpr int32_t version_patch{CHARLS_VERSION_PATCH};

} // namespace charls

#endif
