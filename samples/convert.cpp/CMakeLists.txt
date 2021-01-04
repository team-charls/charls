# Copyright (c) Team CharLS.
# SPDX-License-Identifier: BSD-3-Clause

add_executable(convert-cpp "")

target_sources(convert-cpp
  PRIVATE
    pch.h
    bmp_image.h
    main.cpp
)

if(WIN32 AND MSVC_VERSION GREATER_EQUAL 1920)
  # Only add the manifest file when building a Windows app
  target_sources(convert-cpp PRIVATE app.manifest)
endif()

set_target_properties(convert-cpp PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(convert-cpp PRIVATE charls)