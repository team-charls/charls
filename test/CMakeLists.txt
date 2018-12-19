# Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

add_executable(charlstest "")

target_sources(charlstest
  PRIVATE
    bitstreamdamage.cpp
    bitstreamdamage.h
    compliance.cpp
    compliance.h
    dicomsamples.cpp
    dicomsamples.h
    main.cpp
    performance.cpp
    performance.h
    util.cpp
    util.h
)

target_link_libraries(charlstest PRIVATE charls)
