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
#define CHARLS_RET_MAY_BE_NULL _Ret_maybenull_

// When possible use [[nodiscard]] as this can by handled by all C++17 compilers.
#if defined(__cplusplus) && __cplusplus >= 201703
#define CHARLS_CHECK_RETURN [[nodiscard]]
#else
// Use MSVC specific solution for C and C++14.
#define CHARLS_CHECK_RETURN _Check_return_
#endif

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
#define CHARLS_RET_MAY_BE_NULL

// When possible use [[nodiscard]] as this can by handled by all C++17 compilers.
#if defined(__cplusplus) && __cplusplus >= 201703
    #define CHARLS_CHECK_RETURN [[nodiscard]]
#else
    #if defined(__GNUC__)
        #define CHARLS_CHECK_RETURN __attribute__((warn_unused_result))
    #else
        #define CHARLS_CHECK_RETURN
    #endif
#endif

#endif


#if defined(__GNUC__)
#define CHARLS_ATTRIBUTE(a) __attribute__(a)
#else
#define CHARLS_ATTRIBUTE(a)
#endif
