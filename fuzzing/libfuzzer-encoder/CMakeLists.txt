# SPDX-FileCopyrightText: © 2026 Team CharLS
# SPDX-License-Identifier: BSD-3-Clause

add_executable(libfuzzer-encoder "")

target_sources(libfuzzer-encoder PRIVATE main.cpp)

target_include_directories(libfuzzer-encoder SYSTEM PRIVATE fuzzer)

set_target_properties(libfuzzer-encoder PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(libfuzzer-encoder PRIVATE charls)


if(MSVC)
  # CMake for Ninja gives linker errors. Use the vcxproj file to build the fuzzing targets with MSVC.
  target_compile_options(libfuzzer-encoder PRIVATE "/fsanitize=fuzzer /fsanitize=address")
else()
  target_compile_options(libfuzzer-encoder PRIVATE "-fsanitize=fuzzer,address")
  target_link_options(libfuzzer-encoder PRIVATE "-fsanitize=fuzzer,address")
endif()
