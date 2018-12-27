# Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

# Define the library "charls" target
add_library(charls "")

target_include_directories(charls PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(charls PUBLIC CHARLS_STATIC)
endif()

target_compile_definitions(charls PRIVATE CHARLS_LIBRARY_BUILD)

target_sources(charls
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/charls/api_abi.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/charls/charls.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/charls/jpegls_error.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/charls/public_types.h"
  PRIVATE
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
    "${CMAKE_CURRENT_LIST_DIR}/jpeg_stream_reader.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpeg_stream_writer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/lookup_table.h"
    "${CMAKE_CURRENT_LIST_DIR}/lossless_traits.h"
    "${CMAKE_CURRENT_LIST_DIR}/process_line.h"
    "${CMAKE_CURRENT_LIST_DIR}/scan.h"
    "${CMAKE_CURRENT_LIST_DIR}/util.h"
)

if(CHARLS_INSTALL)
  include(GNUInstallDirs)

  install(TARGETS charls
    CONFIGURATIONS Release
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()
