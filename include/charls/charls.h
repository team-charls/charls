// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once


#include "charls_jpegls_decoder.h"
#include "charls_jpegls_encoder.h"
#include "version.h"


// Undefine CHARLS macros to prevent global macro namespace pollution
#if !defined(CHARLS_LIBRARY_BUILD)
#undef CHARLS_API_IMPORT_EXPORT
#undef CHARLS_NO_DISCARD
#undef CHARLS_FINAL
#undef CHARLS_NOEXCEPT
#undef CHARLS_ATTRIBUTE
#undef CHARLS_DEPRECATED
#undef CHARLS_C_VOID

#undef IN_
#undef IN_OPT_
#undef IN_Z_
#undef IN_READS_BYTES_
#undef OUT_
#undef OUT_OPT_
#undef OUT_WRITES_BYTES_
#undef OUT_WRITES_Z_
#undef RETURN_TYPE_SUCCESS_
#undef CHARLS_CHECK_RETURN
#undef CHARLS_RET_MAY_BE_NULL

#endif
