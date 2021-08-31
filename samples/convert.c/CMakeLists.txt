# Copyright (c) Team CharLS.
# SPDX-License-Identifier: BSD-3-Clause

add_executable(djpls "")

target_sources(djpls
  PRIVATE
    main.c
)

if(WIN32 AND MSVC_VERSION GREATER_EQUAL 1920)
  # Only add the manifest file when building a Windows app
  target_sources(djpls PRIVATE app.manifest)
endif()

target_link_libraries(djpls PRIVATE charls)
install(TARGETS djpls EXPORT charls_targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


if(MSVC)
  # Not all compilers come with the secure C standard extension functions, making it difficult to use them.
  # Disable the MSVC warning about not using these functions to keep the sample code simple and easy to understand.
  target_compile_definitions(djpls PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()