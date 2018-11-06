// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

#include "publictypes.h"
#include "api_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

CHARLS_API_IMPORT_EXPORT const void* CHARLS_API_CALLING_CONVENTION charls_get_error_category();
CHARLS_API_IMPORT_EXPORT const char* CHARLS_API_CALLING_CONVENTION charls_get_error_message(int32_t error_value);

#ifdef __cplusplus
}

#include <system_error>

namespace charls {

class jpegls_error : public std::system_error
{
public:
    explicit jpegls_error(jpegls_errc error_value)
        : system_error(static_cast<int>(error_value), get_error_category())
    {
    }

    jpegls_error(jpegls_errc error_value, const std::string& message)
        : system_error(static_cast<int>(error_value), get_error_category(), message)
    {
    }

    static const std::error_category& get_error_category() noexcept
    {
        return *static_cast<const std::error_category*>(charls_get_error_category());
    }
};

inline std::error_code make_error_code(jpegls_errc error_value)
{
    return {static_cast<int>(error_value), jpegls_error::get_error_category()};
}

} // namespace

#endif
