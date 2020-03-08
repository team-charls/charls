// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "public_types.h"

#ifdef __cplusplus

extern "C" {
#endif

CHARLS_API_IMPORT_EXPORT const void* CHARLS_API_CALLING_CONVENTION charls_get_jpegls_category(void);
CHARLS_API_IMPORT_EXPORT const char* CHARLS_API_CALLING_CONVENTION charls_get_error_message(charls_jpegls_errc error_value);

#ifdef __cplusplus
}

namespace charls {

CHARLS_NO_DISCARD inline const std::error_category& jpegls_category() noexcept
{
    return *static_cast<const std::error_category*>(charls_get_jpegls_category());
}

CHARLS_NO_DISCARD inline std::error_code make_error_code(jpegls_errc error_value) noexcept
{
    return {static_cast<int>(error_value), jpegls_category()};
}


/// <summary>
/// Exception that will be thrown when a called charls method cannot succeed and is allowed to throw.
/// </summary>
class jpegls_error final : public std::system_error
{
public:
    explicit jpegls_error(const std::error_code ec) :
        system_error{ec}
    {
    }

    explicit jpegls_error(const jpegls_errc error_value) :
        system_error{error_value}
    {
    }
};

namespace impl {

#if defined(_MSC_VER)
#define CHARLS_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__)
#define CHARLS_NO_INLINE __attribute__((noinline))
#elif defined(__clang__)
#define CHARLS_NO_INLINE __attribute__((noinline))
#else
#define CHARLS_NO_INLINE
#endif

[[noreturn]] inline CHARLS_NO_INLINE void throw_jpegls_error(const jpegls_errc error_value)
{
    throw jpegls_error(error_value);
}

#undef CHARLS_NO_INLINE

} // namespace impl

inline void check_jpegls_errc(const jpegls_errc error_value)
{
    if (error_value != jpegls_errc::success)
    {
        impl::throw_jpegls_error(error_value); // not inlined by design, as this code path is the exceptional case.
    }
}

} // namespace charls

#endif
