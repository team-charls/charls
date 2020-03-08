# Copyright (c) Team CharLS.
# SPDX-License-Identifier: BSD-3-Clause

add_executable(fuzztest "")

target_sources(fuzztest PRIVATE main.cpp)

set_target_properties(fuzztest PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(fuzztest PRIVATE charls)

if(MSVC)
  # POSIX functions are used, required for AFL.
  target_compile_definitions(fuzztest PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()