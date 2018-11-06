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
    "${CMAKE_CURRENT_SOURCE_DIR}/include/charls/publictypes.h"
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/colortransform.h"
    "${CMAKE_CURRENT_LIST_DIR}/constants.h"
    "${CMAKE_CURRENT_LIST_DIR}/context.h"
    "${CMAKE_CURRENT_LIST_DIR}/contextrunmode.h"
    "${CMAKE_CURRENT_LIST_DIR}/decoderstrategy.h"
    "${CMAKE_CURRENT_LIST_DIR}/defaulttraits.h"
    "${CMAKE_CURRENT_LIST_DIR}/encoderstrategy.h"
    "${CMAKE_CURRENT_LIST_DIR}/interface.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jlscodecfactory.h"
    "${CMAKE_CURRENT_LIST_DIR}/jpegimagedatasegment.h"
    "${CMAKE_CURRENT_LIST_DIR}/jpegls_error.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpegls.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpegmarkercode.h"
    "${CMAKE_CURRENT_LIST_DIR}/jpegmarkersegment.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpegmarkersegment.h"
    "${CMAKE_CURRENT_LIST_DIR}/jpegsegment.h"
    "${CMAKE_CURRENT_LIST_DIR}/jpegstreamreader.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/jpegstreamwriter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/lookuptable.h"
    "${CMAKE_CURRENT_LIST_DIR}/losslesstraits.h"
    "${CMAKE_CURRENT_LIST_DIR}/processline.h"
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