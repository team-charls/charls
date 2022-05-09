// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "public_types.h"

#ifdef __cplusplus
extern "C" {
#endif

CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION charls_validate_spiff_header(
    CHARLS_IN const charls_spiff_header* spiff_header, CHARLS_IN const charls_frame_info* frame_info) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

#ifdef __cplusplus
} // extern "C"
#endif
