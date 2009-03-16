project(charls)

# When user specify build type do not override settings:
IF(NOT CMAKE_BUILD_TYPE)
# The following compiler option are only meant for GCC:
IF(CMAKE_COMPILER_IS_GNUCC)
SET(CMAKE_CXX_FLAGS "-Wall -O3 -g" )
ENDIF(CMAKE_COMPILER_IS_GNUCC)
ENDIF(NOT CMAKE_BUILD_TYPE)

OPTION(charls_BUILD_SHARED_LIBS "Build CharLS with shared libraries." OFF)
SET(BUILD_SHARED_LIBS ${charls_BUILD_SHARED_LIBS})

ADD_SUBDIRECTORY(test)

add_library(CharLS header.cpp  interface.cpp  jpegls.cpp  
  stdafx.cpp
  )
