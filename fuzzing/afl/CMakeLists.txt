# Copyright (c) Team CharLS.
# SPDX-License-Identifier: BSD-3-Clause

add_executable(afl-fuzztest "")

target_sources(afl-fuzztest PRIVATE main.cpp)

set_target_properties(afl-fuzztest PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(afl-fuzztest PRIVATE charls)

if(MSVC)
  # AFL uses POSIX functions: disable warning about potential unsafe methods.
  target_compile_definitions(afl-fuzztest PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
