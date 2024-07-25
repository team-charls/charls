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
#include <optional>

export module charls;

#include "charls_jpegls_decoder.h"
#include "charls_jpegls_encoder.h"
#include "version.h"
