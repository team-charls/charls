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

if(WIN32 AND MSVC)
  # Only add the manifest file when building a Windows app
  target_sources(charls-cli PRIVATE app.manifest)
endif()

set_target_properties(charls-cli PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(charls-cli PRIVATE charls)
