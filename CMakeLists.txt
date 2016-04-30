cmake_minimum_required(VERSION 2.6)
project(charls)

# The following compiler option are only meant for GCC:
if (CMAKE_COMPILER_IS_GNUCC)
    # the following is optimization micromanagement that made better code for x86
    # SET(CMAKE_CXX_FLAGS "-D NDEBUG -O3 -Wall -Wextra -pedantic -fvisibility=hidden -fomit-frame-pointer -momit-leaf-frame-pointer -fweb  -ftracer" )

    # Define GNU C++ defines for both Debug and Release
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra")

    # Define specific Debug settings.
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

    # Define specific Release settings.
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D NDEBUG -O3")
ENDIF ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Define clang C++ defines for both Debug and Release
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra")

    # Define specific Debug settings.
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

    # Define specific Release settings.
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D NDEBUG -O3")
endif()

option (BUILD_SHARED_LIBS "Build CharLS with shared libraries." OFF)
option (BUILD_TESTING     "Build tests"                         ON)

if (WIN32)
   if (BUILD_SHARED_LIBS)
     add_definitions(-D CHARLS_DLL)
   else()
     add_definitions(-D CHARLS_STATIC)
   endif()
endif()

set (charls_PUBLIC_HEADERS src/charls.h src/publictypes.h)

add_library(CharLS src/interface.cpp src/jpegls.cpp src/jpegmarkersegment.cpp src/jpegstreamreader.cpp src/jpegstreamwriter.cpp)
set (CHARLS_LIB_MAJOR_VERSION 2)
set (CHARLS_LIB_MINOR_VERSION 0)
set_target_properties(CharLS PROPERTIES
                             VERSION ${CHARLS_LIB_MAJOR_VERSION}.${CHARLS_LIB_MINOR_VERSION}
                             SOVERSION ${CHARLS_LIB_MAJOR_VERSION})

install (TARGETS CharLS RUNTIME DESTINATION bin
                        LIBRARY DESTINATION lib${LIB_SUFFIX}
                        ARCHIVE DESTINATION lib${LIB_SUFFIX})
install (FILES ${charls_PUBLIC_HEADERS} DESTINATION include/CharLS)

if (BUILD_TESTING)
  add_executable(charlstest test/main.cpp test/time.cpp test/util.cpp test/bitstreamdamage.cpp test/compliance.cpp test/performance.cpp test/dicomsamples.cpp)
  target_link_libraries (charlstest CharLS)
endif ()
