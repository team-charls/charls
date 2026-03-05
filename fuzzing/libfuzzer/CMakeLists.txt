# Copyright (c) Team CharLS.
# SPDX-License-Identifier: BSD-3-Clause

add_executable(libfuzzer-fuzztest "")

target_sources(libfuzzer-fuzztest PRIVATE main.cpp)

set_target_properties(libfuzzer-fuzztest PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(libfuzzer-fuzztest PRIVATE charls)

if(MSVC)
  target_compile_options(libfuzzer-encoder PRIVATE "/fsanitize=fuzzer")
else()
  target_compile_options(libfuzzer-encoder PRIVATE "-fsanitize=fuzzer,address")
  target_link_options(libfuzzer-encoder PRIVATE "-fsanitize=fuzzer,address")
endif()
