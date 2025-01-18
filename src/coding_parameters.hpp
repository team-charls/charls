// SPDX-FileCopyrightText: © 2020 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/public_types.h>

namespace charls {

struct coding_parameters final
{
    int32_t near_lossless;
    uint32_t restart_interval;
    charls::interleave_mode interleave_mode;
    color_transformation transformation;
};

} // namespace charls
