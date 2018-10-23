# Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

add_executable(convert.c "")

target_sources(convert.c
  PRIVATE
    main.c
)

target_link_libraries(convert.c PRIVATE charls)