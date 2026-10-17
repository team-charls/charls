# SPDX-FileCopyrightText: © 2018 Team CharLS
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

  # Define that compiler provided conformance to the C standard. Introduced with VS 2022 17.2
  if(MSVC_VERSION GREATER_EQUAL 1932)
    add_compile_options("/Zc:__STDC__")
  endif()
endif()
