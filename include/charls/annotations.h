// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#ifdef _MSC_VER

#include <sal.h>

// Note: these macro's are not prefixed with CHARLS_, as these macro's are used for function parameters.
//       and long macro's would make the code harder to read.
#define CHARLS_IN _In_
#define CHARLS_IN_OPT _In_opt_
#define CHARLS_IN_Z _In_z_
#define CHARLS_IN_READS_BYTES(size) _In_reads_bytes_(size)
#define CHARLS_OUT _Out_
#define CHARLS_OUT_OPT _Out_opt_
#define CHARLS_OUT_WRITES_BYTES(size) _Out_writes_bytes_(size)
#define CHARLS_OUT_WRITES_Z(size_in_bytes) _Out_writes_z_(size_in_bytes)
#define CHARLS_RETURN_TYPE_SUCCESS(expr) _Return_type_success_(expr)
#define CHARLS_CHECK_RETURN _Check_return_
#define CHARLS_RET_MAY_BE_NULL _Ret_maybenull_

#else

#define CHARLS_IN
#define CHARLS_IN_OPT
#define CHARLS_IN_Z
#define CHARLS_IN_READS_BYTES(size)
#define CHARLS_OUT
#define CHARLS_OUT_OPT
#define CHARLS_OUT_WRITES_BYTES(size)
#define CHARLS_OUT_WRITES_Z(size_in_bytes)
#define CHARLS_RETURN_TYPE_SUCCESS(expr)
#define CHARLS_CHECK_RETURN
#define CHARLS_RET_MAY_BE_NULL

#endif


#if defined(__GNUC__)

#define CHARLS_ATTRIBUTE(a) __attribute__(a)

#else

#define CHARLS_ATTRIBUTE(a)

#endif
