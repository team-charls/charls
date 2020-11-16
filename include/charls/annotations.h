// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#ifdef _MSC_VER

#include <sal.h>

// Note: these macro's are not prefixed with CHARLS_, as these macro's are used for function parameters.
//       and long macro's would make the code harder to read.
#define IN_ _In_
#define IN_OPT_ _In_opt_
#define IN_Z_ _In_z_
#define IN_READS_BYTES_(size) _In_reads_bytes_(size)
#define OUT_ _Out_
#define OUT_OPT_ _Out_opt_
#define OUT_WRITES_BYTES_(size) _Out_writes_bytes_(size)
#define OUT_WRITES_Z_(size_in_bytes) _Out_writes_z_(size_in_bytes)
#define RETURN_TYPE_SUCCESS_(expr) _Return_type_success_(expr)
#define CHARLS_CHECK_RETURN _Check_return_
#define CHARLS_RET_MAY_BE_NULL _Ret_maybenull_

#else

#define IN_
#define IN_OPT_
#define IN_Z_
#define IN_READS_BYTES_(size)
#define OUT_
#define OUT_OPT_
#define OUT_WRITES_BYTES_(size)
#define OUT_WRITES_Z_(size_in_bytes)
#define RETURN_TYPE_SUCCESS_(expr)
#define CHARLS_CHECK_RETURN
#define CHARLS_RET_MAY_BE_NULL

#endif


#if defined(__clang__) || defined(__GNUC__)

#define CHARLS_ATTRIBUTE(a) __attribute__(a)

#else

#define CHARLS_ATTRIBUTE(a)

#endif
