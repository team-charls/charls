# SPDX-FileCopyrightText: © 2018 Team CharLS
# SPDX-License-Identifier: BSD-3-Clause

# Define the library "charls" target
add_library(charls "")

if(MSVC)
  if(MSVC_VERSION GREATER_EQUAL 1910)
    # https://github.com/team-charls/charls/issues/278
    set_target_properties(charls PROPERTIES INTERFACE_COMPILE_OPTIONS "/permissive-")
  endif()
endif()

# Enable IPO (Interprocedural optimization) for shared lib in release mode with GCC for better performance.
# Note: MSVC and Clang will create with IPO larger binaries that are not significant faster.
if(BUILD_SHARED_LIBS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_supported)
  if(ipo_supported)
      message("-- IPO is supported and used")
      set_property(TARGET charls PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  else()
      message("-- IPO is not supported")
  endif()
endif()

target_include_directories(charls PUBLIC
  $<BUILD_INTERFACE:${charls_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(charls PUBLIC CHARLS_STATIC)
endif()

target_precompile_headers(charls PRIVATE ${CMAKE_CURRENT_LIST_DIR}/pch.hpp)

# Use different DLL filenames on Windows. It allows platform neutral adapters to load dynamically the correct version.
if(WIN32 AND BUILD_SHARED_LIBS)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID MATCHES "ARM64")
      set_target_properties(charls PROPERTIES OUTPUT_NAME "charls-3-arm64")
    else()
      set_target_properties(charls PROPERTIES OUTPUT_NAME "charls-3-x64")
    endif()
  else()
    set_target_properties(charls PROPERTIES OUTPUT_NAME "charls-3-x86")
  endif()
endif()

# Disable C++ run-time type info (dynamic cast + typeid). CharLS doesn't use
# this functionality and disabling it will result in a smaller image.
# Don't do this for gcc and clang (normally done with -fno-rtti) as it will cause
# UBSAN warnings as jpegls_category is derived from std::error_category
# If the base class was built with RTTI and has virtual functions, the derived class
# also needs to be build with RTTI.
if(MSVC)
  target_compile_options(charls PRIVATE /GR-)
endif()

if(CHARLS_ENABLE_ASAN)
  target_compile_options(charls PRIVATE -fsanitize=address)
  target_link_libraries(charls PRIVATE -fsanitize=address)
endif()

set_target_properties(charls PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR})

target_compile_definitions(charls PRIVATE CHARLS_LIBRARY_BUILD)

# CharLS requires C++17 or newer.
target_compile_features(charls PUBLIC cxx_std_17)

set(HEADERS
    "include/charls/annotations.h"
    "include/charls/api_abi.h"
    "include/charls/charls.h"
    "include/charls/charls.hpp"
    "include/charls/charls_jpegls_decoder.h"
    "include/charls/charls_jpegls_encoder.h"
    "include/charls/jpegls_decoder.hpp"
    "include/charls/jpegls_encoder.hpp"
    "include/charls/jpegls_error.h"
    "include/charls/jpegls_error.hpp"
    "include/charls/public_types.h"
    "include/charls/undef_macros.h"
    "include/charls/validate_spiff_header.h"
    "include/charls/version.h"
)
foreach(header HEADERS)
  list(APPEND CHARLS_PUBLIC_HEADER $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${header}>)
  list(APPEND CHARLS_PUBLIC_HEADER $<INSTALL_INTERFACE:${header}>)
endforeach()

set_target_properties(charls PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(charls PROPERTIES VISIBILITY_INLINES_HIDDEN 1)
set_property(TARGET charls PROPERTY PUBLIC_HEADER ${HEADERS})

set_target_properties(charls PROPERTIES
  LINK_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/charls.version)
if(UNIX AND NOT APPLE)
  set_property(TARGET charls APPEND_STRING PROPERTY
    LINK_FLAGS " -Wl,--version-script=${CMAKE_CURRENT_LIST_DIR}/charls.version")
endif()

target_sources(charls
  PUBLIC
    ${CHARLS_PUBLIC_HEADERS}
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/assert.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/charls_jpegls_decoder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/charls_jpegls_encoder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding_parameters.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/color_transform.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/conditional_static_cast.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/constants.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/copy_from_line_buffer.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/copy_to_line_buffer.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/default_traits.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/golomb_lut.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/golomb_lut.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpeg_marker_code.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpeg_stream_reader.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpeg_stream_reader.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpeg_stream_writer.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpeg_stream_writer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpegls_algorithm.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpegls_error.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpegls_preset_coding_parameters.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpegls_preset_parameters_type.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/lossless_traits.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/make_scan_codec.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/make_scan_codec.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/pch.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/quantization_lut.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/quantization_lut.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/regular_mode_context.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/run_mode_context.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/scan_codec.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/scan_decoder.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/scan_decoder_impl.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/scan_encoder.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/scan_encoder_impl.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/span.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/util.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/validate_spiff_header.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/version.cpp"
)

if(WIN32 AND BUILD_SHARED_LIBS)
  # Only add the Win32 resource script file when building a DLL
  target_sources(charls PRIVATE "${CMAKE_CURRENT_LIST_DIR}/charls.rc")
endif()

if(CHARLS_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  install(TARGETS charls EXPORT charls_targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/charls
  )

  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/charlsConfigVersion.cmake COMPATIBILITY SameMajorVersion)

  # Configure and copy the pkg-config configuration file.
  # These type of configuration file can make it easier to detect if charls is installed.
  CONFIGURE_FILE("${CMAKE_CURRENT_LIST_DIR}/charls-template.pc" "charls.pc" @ONLY)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/charls.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/charlsConfigVersion.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/charls")

  install(EXPORT charls_targets FILE charlsConfig.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/charls)
endif()
