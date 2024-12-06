// SPDX-FileCopyrightText: © 2023 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

module;

#define CHARLS_BUILD_AS_CPP_MODULE

#include <version>
#include <sal.h>

#ifndef __cpp_lib_modules

#include <cstddef>
#include <cstdint>
#include <system_error>
#include <functional>
#include <memory>
#include <utility>

#endif

export module charls;

#ifdef __cpp_lib_modules

import std;

#endif

#include "jpegls_decoder.hpp"
#include "jpegls_encoder.hpp"
#include "version.h"
