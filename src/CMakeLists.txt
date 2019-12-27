# Copyright (c) Team CharLS.
# SPDX-License-Identifier: BSD-3-Clause

# Define the library "charls" target
add_library(charls "")

target_include_directories(charls PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(charls PUBLIC CHARLS_STATIC)
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(charls PROPERTIES OUTPUT_NAME "charls-2-x64")
  else()
    set_target_properties(charls PROPERTIES OUTPUT_NAME "charls-2-x86")
  endif()
endif()

set_target_properties(charls PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR})

target_compile_definitions(charls PRIVATE CHARLS_LIBRARY_BUILD)

set(CHARLS_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/charls/api_abi.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/charls/charls.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/charls/charls_legacy.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/charls/jpegls_error.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/charls/public_types.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/charls/version.h"
)

set_target_properties(charls PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_property(TARGET charls PROPERTY PUBLIC_HEADER ${CHARLS_PUBLIC_HEADERS})

target_sources(charls
  PUBLIC
    ${CHARLS_PUBLIC_HEADERS}
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/charls_jpegls_decoder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/charls_jpegls_encoder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/color_transform.h"
    "${CMAKE_CURRENT_LIST_DIR}/constants.h"
    "${CMAKE_CURRENT_LIST_DIR}/context.h"
    "${CMAKE_CURRENT_LIST_DIR}/context_run_mode.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoder_strategy.h"
    "${CMAKE_CURRENT_LIST_DIR}/default_traits.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoder_strategy.h"
    "${CMAKE_CURRENT_LIST_DIR}/interface.cpp"
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
    "${CMAKE_CURRENT_LIST_DIR}/version.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/charls.def"
    "${CMAKE_CURRENT_LIST_DIR}/charls.rc"
)

if(CHARLS_INSTALL)
  include(GNUInstallDirs)

  install(TARGETS charls
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/charls
  )
endif()
