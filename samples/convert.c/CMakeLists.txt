# Copyright (c) Team CharLS.
# SPDX-License-Identifier: BSD-3-Clause

add_executable(convert-c "")

target_sources(convert-c
  PRIVATE
    main.c
)

if(WIN32 AND MSVC_VERSION GREATER_EQUAL 1920)
  # Only add the manifest file when building a Windows app
  target_sources(convert-c PRIVATE app.manifest)
endif()

target_link_libraries(convert-c PRIVATE charls)

if(MSVC)
  # Not all compilers come with the secure C standard extension functions, making it difficult to use them.
  # Disable the MSVC warning about not using these functions to keep the sample code simple and easy to understand.
  target_compile_definitions(convert-c PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()