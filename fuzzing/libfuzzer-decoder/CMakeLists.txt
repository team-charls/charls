# SPDX-FileCopyrightText: © 2023 Team CharLS
# SPDX-License-Identifier: BSD-3-Clause

add_executable(libfuzzer-decoder "")

target_sources(libfuzzer-decoder PRIVATE main.cpp)

set_target_properties(libfuzzer-decoder PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(libfuzzer-decoder PRIVATE charls)

if(MSVC)
  target_compile_options(libfuzzer-decoder PRIVATE "/fsanitize=fuzzer")
else()
  target_compile_options(libfuzzer-decoder PRIVATE "-fsanitize=fuzzer,address")
  target_link_options(libfuzzer-decoder PRIVATE "-fsanitize=fuzzer,address")
endif()
