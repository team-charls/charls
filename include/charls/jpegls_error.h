// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

#include "public_types.h"
#include "api_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

CHARLS_API_IMPORT_EXPORT const void* CHARLS_API_CALLING_CONVENTION charls_jpegls_category(void);
CHARLS_API_IMPORT_EXPORT const char* CHARLS_API_CALLING_CONVENTION charls_get_error_message(int32_t error_value);

#ifdef __cplusplus
}

#include <system_error>

namespace charls {

CHARLS_NO_DISCARD inline const std::error_category& jpegls_category() noexcept
{
    return *static_cast<const std::error_category*>(charls_jpegls_category());
}

CHARLS_NO_DISCARD inline std::error_code make_error_code(jpegls_errc error_value) noexcept
{
    return {static_cast<int>(error_value), jpegls_category()};
}


/// <summary>
/// Exception that will be thrown when a called charls method cannot succeed and is allowed to throw.
/// </summary>
class jpegls_error : public std::system_error
{
public:
    explicit jpegls_error(std::error_code ec)
        : system_error{ec}
    {
    }

    explicit jpegls_error(jpegls_errc error_value)
        : system_error{error_value}
    {
    }
};

} // namespace charls

#endif
