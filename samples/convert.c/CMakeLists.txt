# Copyright (c) Team CharLS.
# SPDX-License-Identifier: BSD-3-Clause

add_executable(convert-c "")

target_sources(convert-c
  PRIVATE
    main.c
)

target_link_libraries(convert-c PRIVATE charls)