// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

// Undefine CHARLS macros to prevent global macro namespace pollution and usage by client code.
// The macros are not part of the official API.
#undef CHARLS_API_IMPORT_EXPORT
#undef CHARLS_FINAL
#undef CHARLS_NOEXCEPT
#undef CHARLS_ATTRIBUTE
#undef CHARLS_C_VOID
#undef CHARLS_STD
#undef CHARLS_NO_INLINE
#undef CHARLS_IN
#undef CHARLS_IN_OPT
#undef CHARLS_IN_Z
#undef CHARLS_IN_READS_BYTES
#undef CHARLS_OUT
#undef CHARLS_OUT_OPT
#undef CHARLS_OUT_WRITES_BYTES
#undef CHARLS_OUT_WRITES_Z
#undef CHARLS_RETURN_TYPE_SUCCESS
#undef CHARLS_CHECK_RETURN
#undef CHARLS_RET_MAY_BE_NULL
