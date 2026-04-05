# SPDX-FileCopyrightText: © 2026 Team CharLS
# SPDX-License-Identifier: BSD-3-Clause

add_executable(charls-cli "")

target_sources(charls-cli
  PRIVATE
    main.cpp
    benchmark.cpp
    benchmark.hpp
    compare.cpp
    compare.hpp
    decode.cpp
    decode.hpp
    encode.cpp
    encode.hpp
    pch.hpp
    utility.cpp
    utility.hpp
)

target_precompile_headers(charls-cli PRIVATE ${CMAKE_CURRENT_LIST_DIR}/pch.hpp)

# Use argparse provided by vcpkg. Prefer system/vcpkg first
# and fall back to fetching the project so builds work without vcpkg configured.
find_package(argparse QUIET)
if(NOT argparse_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG 3eda91b2e1ce7d569f84ba295507c4cd8fd96910
  )
  FetchContent_MakeAvailable(argparse)
endif()

if(WIN32 AND MSVC)
  # Only add the manifest file when building a Windows app
  target_sources(charls-cli PRIVATE app.manifest)
endif()

set_target_properties(charls-cli PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(charls-cli PRIVATE charls)
target_link_libraries(charls-cli PRIVATE argparse::argparse)
