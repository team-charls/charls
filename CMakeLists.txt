project(charls)

SET(CMAKE_CXX_FLAGS "-Wall -O3 -g" )  

add_library(CharLS SHARED header.cpp  interface.cpp  jpegls.cpp  
  stdafx.cpp
  )
