# Copyright (c) Team CharLS.
# SPDX-License-Identifier: BSD-3-Clause

add_executable(convert-cpp "")

target_sources(convert-cpp
  PRIVATE
    pch.h
    bmp_image.h
    main.cpp
)

target_link_libraries(convert-cpp PRIVATE charls)