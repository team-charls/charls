// SPDX-FileCopyrightText: © 2019 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include <charls/version.h>

#include "util.hpp"

using namespace charls;

extern "C" {

const char* CHARLS_API_CALLING_CONVENTION charls_get_version_string()
{
    return TO_STRING(CHARLS_VERSION_MAJOR) "." TO_STRING(CHARLS_VERSION_MINOR) "." TO_STRING(CHARLS_VERSION_PATCH);
}

USE_DECL_ANNOTATIONS void CHARLS_API_CALLING_CONVENTION charls_get_version_number(int32_t* major, int32_t* minor,
                                                                                  int32_t* patch)
{
    if (major)
    {
        *major = version_major;
    }

    if (minor)
    {
        *minor = version_minor;
    }

    if (patch)
    {
        *patch = version_patch;
    }
}
}
