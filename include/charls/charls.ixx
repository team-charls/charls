// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

module;

#define CHARLS_BUILD_AS_CPP_MODULE

#include <sal.h>
#include <cstddef>
#include <cstdint>
#include <system_error>
#include <functional>
#include <memory>
#include <utility>

export module charls;

#include "jpegls_decoder.hpp"
#include "jpegls_encoder.hpp"
#include "version.h"
