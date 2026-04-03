# Copyright (c) Team CharLS.
# SPDX-License-Identifier: BSD-3-Clause

add_executable(charls-cli "")

target_sources(charls-cli
  PRIVATE
    main.cpp
    performance.cpp
    performance.hpp
    util.cpp
    util.hpp
)

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
