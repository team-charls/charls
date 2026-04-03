# SPDX-FileCopyrightText: © 2023 Team CharLS
# SPDX-License-Identifier: BSD-3-Clause

add_executable(afl-fuzz-test "")

target_sources(afl-fuzz-test PRIVATE main.cpp)

set_target_properties(afl-fuzz-test PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(afl-fuzz-test PRIVATE charls)

if(MSVC)
  # AFL uses POSIX functions: disable warning about potential unsafe methods.
  target_compile_definitions(afl-fuzz-test PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
