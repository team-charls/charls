project(charls)
cmake_minimum_required(VERSION 2.6)

# The following compiler option are only meant for GCC:
IF(CMAKE_COMPILER_IS_GNUCC)

# the following is optimization micromanagement that made better code for x86
# SET(CMAKE_CXX_FLAGS "-D NDEBUG -O3 -Wall -Wextra -pedantic -fvisibility=hidden -fomit-frame-pointer -momit-leaf-frame-pointer -fweb  -ftracer" )

# Define GNU C++ defines for both Debug and Release
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

# Define specific Debug settings.
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# Define specific Release settings.
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D NDEBUG -O3")

ENDIF(CMAKE_COMPILER_IS_GNUCC)

OPTION(charls_BUILD_SHARED_LIBS "Build CharLS with shared libraries." OFF)
SET(BUILD_SHARED_LIBS ${charls_BUILD_SHARED_LIBS})

if(WIN32)
   if(BUILD_SHARED_LIBS)
     add_definitions(-D CHARLS_DLL)
   else(BUILD_SHARED_LIBS)
     add_definitions(-D CHARLS_STATIC)
   endif(BUILD_SHARED_LIBS)
endif(WIN32)

add_library(CharLS src/interface.cpp src/jpegls.cpp src/jpegmarkersegment.cpp src/jpegstreamreader.cpp src/jpegstreamwriter.cpp)

add_executable(charlstest test/main.cpp test/time.cpp test/util.cpp test/bitstreamdamage.cpp test/compliance.cpp test/performance.cpp test/dicomsamples.cpp)
target_link_libraries (charlstest CharLS)

