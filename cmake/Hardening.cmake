# SPDX-FileCopyrightText: © 2026 Team CharLS
# SPDX-License-Identifier: BSD-3-Clause

# Enables C++ standard library hardening (the C++26 "hardened implementation"
# concept) for a target, across the major standard library implementations:

include_guard(GLOBAL)

function(enable_cpp_standard_library_hardening target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "enable_cpp_standard_library_hardening: '${target}' is not a target")
  endif()

  # Apply standard library hardening flags based on the active compiler
  # Assume a 1:1 relationship between compiler and the used standard library
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_definitions(${target} PRIVATE _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_compile_definitions(${target} PRIVATE _GLIBCXX_ASSERTIONS)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      target_compile_definitions(${target} PRIVATE _MSVC_STL_HARDENING=1)
  endif()

  message(STATUS "C++ Standard Library hardening for '${target}'")
endfunction()
