// SPDX-FileCopyrightText: © 2016 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>

void decode_performance_tests(const char* filename, std::uint32_t loop_count);
void encode_performance_tests(const char* filename, std::uint32_t loop_count);
