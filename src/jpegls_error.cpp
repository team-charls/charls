// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include <charls/jpegls_error.h>

namespace charls {

class jpegls_category : public std::error_category
{
public:
    const char* name() const noexcept override
    {
        return "charls::jpegls";
    }

    std::string message(int error_value) const override
    {
        return charls_get_error_message(error_value);
    }
};

}

const void* CHARLS_API_CALLING_CONVENTION charls_get_error_category()
{
    static charls::jpegls_category instance;
    return &instance;
}

const char* CHARLS_API_CALLING_CONVENTION charls_get_error_message(int32_t /*error_value*/)
{
    return "";
}
