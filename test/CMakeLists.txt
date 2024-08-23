# Copyright (c) Team CharLS.
# SPDX-License-Identifier: BSD-3-Clause

add_executable(charlstest "")

target_sources(charlstest
  PRIVATE
    bitstreamdamage.cpp
    bitstreamdamage.hpp
    compliance.cpp
    compliance.hpp
    main.cpp
    performance.cpp
    performance.hpp
    util.cpp
    util.hpp
)

if(WIN32 AND MSVC_VERSION GREATER_EQUAL 1920)
  # Only add the manifest file when building a Windows app
  target_sources(charlstest PRIVATE app.manifest)
endif()

set_target_properties(charlstest PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_libraries(charlstest PRIVATE charls)
