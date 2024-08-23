// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "public_types.h"

#ifdef __cplusplus
extern "C" {
#endif

CHARLS_API_IMPORT_EXPORT const char* CHARLS_API_CALLING_CONVENTION charls_get_error_message(charls_jpegls_errc error_value);

#ifdef __cplusplus
}
#endif
