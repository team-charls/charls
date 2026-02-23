# SPDX-FileCopyrightText: © 2009 Team CharLS
# SPDX-License-Identifier: BSD-3-Clause

add_executable(charls-test "")

target_sources(charls-test
  PRIVATE
    bit_stream_damage.cpp
    bit_stream_damage.hpp
    compliance.cpp
    compliance.hpp
    main.cpp
    performance.cpp
    performance.hpp
    util.cpp
    util.hpp
    portable_anymap_file.hpp
)

if(WIN32 AND MSVC_VERSION GREATER_EQUAL 1920)
  # Only add the manifest file when building a Windows app
  target_sources(charls-test PRIVATE app.manifest)
endif()

set_target_properties(charls-test PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(charls-test PRIVATE charls)
