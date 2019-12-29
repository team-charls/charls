// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <charls/version.h>

// Turn A into a string literal without expanding macro definitions
// (however, if invoked from a macro, macro arguments are expanded).
#define TO_STRING_NX(A) #A

// Turn A into a string literal after macro-expanding it.
#define TO_STRING(A) TO_STRING_NX(A)

const char* CHARLS_API_CALLING_CONVENTION charls_get_version_string() noexcept
{
    return TO_STRING(CHARLS_VERSION_MAJOR) "." TO_STRING(CHARLS_VERSION_MINOR) "." TO_STRING(CHARLS_VERSION_PATCH);
}

void CHARLS_API_CALLING_CONVENTION charls_get_version_number(int32_t* major, int32_t* minor, int32_t* patch) noexcept
{
    if (major)
    {
        *major = CHARLS_VERSION_MAJOR;
    }

    if (minor)
    {
        *minor = CHARLS_VERSION_MINOR;
    }

    if (patch)
    {
        *patch = CHARLS_VERSION_PATCH;
    }
}
