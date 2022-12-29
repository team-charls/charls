# Copyright (c) Team CharLS.
# SPDX-License-Identifier: BSD-3-Clause

# Define the library "charls" target
add_library(charls "")

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

# Use different DLL filenames on Windows. It allows platform neutral adapters to load dynamicly the correct version.
if(WIN32 AND BUILD_SHARED_LIBS)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID MATCHES "ARM64")
      set_target_properties(charls PROPERTIES OUTPUT_NAME "charls-2-arm64")
    else()
      set_target_properties(charls PROPERTIES OUTPUT_NAME "charls-2-x64")
    endif()
  else()
    set_target_properties(charls PROPERTIES OUTPUT_NAME "charls-2-x86")
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

set_target_properties(charls PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR})

target_compile_definitions(charls PRIVATE CHARLS_LIBRARY_BUILD)
# CharLS requires C++14 or newer.
target_compile_features(charls PUBLIC cxx_std_14)

set(HEADERS
    "include/charls/api_abi.h"
    "include/charls/annotations.h"
    "include/charls/charls.h"
    "include/charls/charls_jpegls_decoder.h"
    "include/charls/charls_jpegls_encoder.h"
    "include/charls/jpegls_error.h"
    "include/charls/public_types.h"
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
    "${CMAKE_CURRENT_LIST_DIR}/byte_span.h"
    "${CMAKE_CURRENT_LIST_DIR}/charls_jpegls_decoder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/charls_jpegls_encoder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/coding_parameters.h"
    "${CMAKE_CURRENT_LIST_DIR}/color_transform.h"
    "${CMAKE_CURRENT_LIST_DIR}/conditional_static_cast.h"
    "${CMAKE_CURRENT_LIST_DIR}/constants.h"
    "${CMAKE_CURRENT_LIST_DIR}/context_regular_mode.h"
    "${CMAKE_CURRENT_LIST_DIR}/context_run_mode.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder_strategy.h"
    "${CMAKE_CURRENT_LIST_DIR}/default_traits.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder_strategy.h"
    "${CMAKE_CURRENT_LIST_DIR}/jls_codec_factory.h"
    "${CMAKE_CURRENT_LIST_DIR}/jpegls_error.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpegls.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpeg_marker_code.h"
    "${CMAKE_CURRENT_LIST_DIR}/jpegls_preset_coding_parameters.h"
    "${CMAKE_CURRENT_LIST_DIR}/jpegls_preset_parameters_type.h"
    "${CMAKE_CURRENT_LIST_DIR}/jpeg_stream_reader.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpeg_stream_writer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/lookup_table.h"
    "${CMAKE_CURRENT_LIST_DIR}/lossless_traits.h"
    "${CMAKE_CURRENT_LIST_DIR}/process_line.h"
    "${CMAKE_CURRENT_LIST_DIR}/scan.h"
    "${CMAKE_CURRENT_LIST_DIR}/util.h"
    "${CMAKE_CURRENT_LIST_DIR}/validate_spiff_header.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/version.cpp"
)

if(WIN32 AND BUILD_SHARED_LIBS)
  # Only add the Win32 resource script file when building a DLL
  target_sources(charls PRIVATE "${CMAKE_CURRENT_LIST_DIR}/charls.rc")

  # Only add the definition file when building a x86 DLL with MSVC
  # The definition file is needed to ensure the legacy functions are exported with the correct name
  if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_sources(charls PRIVATE "${CMAKE_CURRENT_LIST_DIR}/charls.def")
  endif()
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
