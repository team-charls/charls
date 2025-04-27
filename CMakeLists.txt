# SPDX-FileCopyrightText: © 2009 Team CharLS
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16...4.01)

# Extract the version info from version.h
file(READ "include/charls/version.h" version)
string(REGEX MATCH "CHARLS_VERSION_MAJOR ([0-9]*)" _ ${version})
set(version_major ${CMAKE_MATCH_1})
string(REGEX MATCH "CHARLS_VERSION_MINOR ([0-9]*)" _ ${version})
set(version_minor ${CMAKE_MATCH_1})
string(REGEX MATCH "CHARLS_VERSION_PATCH ([0-9]*)" _ ${version})
set(version_patch ${CMAKE_MATCH_1})
message(STATUS "CharLS version: ${version_major}.${version_minor}.${version_patch}")

project(charls VERSION ${version_major}.${version_minor}.${version_patch} LANGUAGES C CXX)

# Determine if project is built as a sub-project (using add_subdirectory) or if it is the main project.
set(MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MAIN_PROJECT ON)
  message(STATUS "Building as main project, CMake version: ${CMAKE_VERSION}")
endif()

# The basic options to control what is build extra.
option(CHARLS_BUILD_TESTS "Build test application" ${MAIN_PROJECT})
option(CHARLS_BUILD_AFL_FUZZ_TEST "Build AFL test fuzzer application" ${MAIN_PROJECT})
option(CHARLS_BUILD_LIBFUZZER_FUZZ_TEST "Build LibFuzzer test fuzzer application" ${MAIN_PROJECT})
option(CHARLS_BUILD_SAMPLES "Build sample applications" ${MAIN_PROJECT})
option(CHARLS_INSTALL "Generate the install target." ${MAIN_PROJECT})

# Provide BUILD_SHARED_LIBS as an option for GUI tools
option(BUILD_SHARED_LIBS "Will control if charls lib is build as shared lib/DLL or static library")

# Provide option to build CharLS with address sanitizer
option(CHARLS_ENABLE_ASAN "Build with address sanitizer enabled." OFF)

# Provide option to build CharLS with clang-tidy
option(CHARLS_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis." OFF)

# These options are used by the CI pipeline to ensure new warnings are detected quickly.
# Not enabled by default to ensure the CharLS package is end-user friendly.
option(CHARLS_PEDANTIC_WARNINGS "Enable extra warnings and static analysis." OFF)
option(CHARLS_TREAT_WARNING_AS_ERROR "Treat a warning as an error." OFF)

# CharLS is written in portable c++:
set(CMAKE_CXX_EXTENSIONS OFF)

# Configure the supported C++ compilers: gcc, clang and MSVC
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(PEDANTIC_CXX_COMPILE_FLAGS
    -Wall
    -Wextra
    -pedantic
    -pedantic-errors
    -Wold-style-cast
    -Wfloat-equal
    -Wlogical-op
    -Wundef
    -Wredundant-decls
    -Wshadow
    -Wwrite-strings
    -Wpointer-arith
    -Wcast-qual
    -Wformat=2
    -Wmissing-include-dirs
    -Wcast-align
    -Wctor-dtor-privacy
    -Wdisabled-optimization
    -Winvalid-pch
    -Woverloaded-virtual
    -Wnon-virtual-dtor
    -Wnoexcept
    -Wdouble-promotion
    -Wtrampolines
    -Wzero-as-null-pointer-constant
    -Wuseless-cast
    -Wvector-operation-performance
    -Wsized-deallocation
    -Wattributes
    -Wuseless-cast
    -Wconversion
  )
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
      set(PEDANTIC_CXX_COMPILE_FLAGS ${PEDANTIC_CXX_COMPILE_FLAGS}
        -Wshift-overflow=2
        -Wnull-dereference
        -Wduplicated-cond
      )
  endif()
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
      set(PEDANTIC_CXX_COMPILE_FLAGS ${PEDANTIC_CXX_COMPILE_FLAGS}
        -Wcast-align=strict
      )
  endif()

  set(WARNINGS_AS_ERRORS_FLAG_COMPILER -Werror)
  set(WARNINGS_AS_ERRORS_FLAG_LINKER LINKER:--fatal-warnings)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(PEDANTIC_CXX_COMPILE_FLAGS
      -Wall
      -Wextra # (-W is synonym)
      -Wnon-gcc
      -Wpedantic
      -Wcast-qual
      -Wformat=2
      -Wvla
      -Warray-bounds-pointer-arithmetic
      -Wassign-enum
      -Wbad-function-cast
      -Wconditional-uninitialized
      -Widiomatic-parentheses
      -Wimplicit-fallthrough
      -Wloop-analysis
      -Wpointer-arith
      -Wshift-sign-overflow
      -Wtautological-constant-in-range-compare
      -Wunreachable-code-aggressive
      -Wthread-safety
      -Wthread-safety-beta
      -Wcomma
      -Wconversion
      # -Weverything provides the option to discover useful Clang warnings.
      # The list below ignores not useful Weverything warnings.
      -Wno-weak-vtables                       # Ignore, linker will remove the couple of extra vtables.
      -Wno-padded                             # Ignore, padding optimization is not needed.
      -Wno-c++98-compat                       # Ignore, CharLS targets C++17, ignore C++98 compatibility.
      -Wno-c++98-compat-pedantic              # Ignore, CharLS targets C++17, ignore C++98 compatibility.
      -Wno-global-constructors                # Ignore, by design CharLS uses types created at startup.
      -Wno-switch-enum                        # Ignore, cases are handled by default.
      -Wno-sign-conversion                    # Ignore, would just introduce ugly static_asserts.
      -Wno-exit-time-destructors              # Ignore, by design exit-time destructors are used.
      -Wno-missing-braces                     # Ignore, False warning in clang 5.0, fixed in 6.0.
  )

  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0.2)
    set(PEDANTIC_CXX_COMPILE_FLAGS ${PEDANTIC_CXX_COMPILE_FLAGS}
      -Wno-undefined-func-template            # Ignore, linker will complain if final template code is not available.
    )
  endif()
  if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
    set(PEDANTIC_CXX_COMPILE_FLAGS ${PEDANTIC_CXX_COMPILE_FLAGS}
      -Walloca
    )
  endif()

  set(WARNINGS_AS_ERRORS_FLAG_COMPILER -Werror)
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
    set(WARNINGS_AS_ERRORS_FLAG_LINKER LINKER:--fatal-warnings)
  endif()

  if(NOT APPLE)
    set(LIBFUZZER_SUPPORTED 1)
  endif()
endif()

if(MSVC)
  set(PEDANTIC_CXX_COMPILE_FLAGS /W4)
  set(WARNINGS_AS_ERRORS_FLAG_COMPILER /WX)
  set(WARNINGS_AS_ERRORS_FLAG_LINKER /WX)

  # Exception settings are not set for ARM(64) when building for Ninja
  string(TOLOWER ${CMAKE_CXX_COMPILER} CXX_COMPILER_LOWER)
  string(FIND ${CXX_COMPILER_LOWER} "arm" ARM_DETECTED)
  if(${ARM_DETECTED} GREATER 0)
    add_compile_options("/EHsc")
  endif()

  # Remove option /GR (/GR is added by default by CMake). /GR is already the default
  # and this makes it possible to use /GR- without warnings.
  string(REGEX REPLACE " /GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

  # All C and C++ source files are in utf-8 without signature (bom), MSVC requires utf-8 switch to read files correctly.
  add_compile_options("/utf-8")

  # Zc:__cplusplus: Will configure the MSVC compiler to use the correct value for the __cplusplus macro
  # This option is introduced with Visual Studio 2017 version 15.7
  if(MSVC_VERSION GREATER_EQUAL 1914)
    add_compile_options("/Zc:__cplusplus")
  endif()

  # /Zc:throwingNew: Will configure the MSVC compiler that only the standard throwing operator new is used.
  add_compile_options("/Zc:throwingNew")

  # /permissive-: Will configure the MSVC compiler to accept only standards conformance C++
  # This option is introduced with Visual Studio 2017.
  if(MSVC_VERSION GREATER_EQUAL 1910)
    add_compile_options("/permissive-")
  endif()

  # /ZH:SHA_256: Will use SHA 256 for checksums between source code and debug symbols (more secure)
  add_compile_options("/ZH:SHA_256")

  # /guard:cf: Will enable Control Flow Guard, which provides addition compile time and runtime checks.
  add_compile_options("/guard:cf")
  add_link_options("/guard:cf")

  # /CETCOMPAT: Mark executable image compatible with the Control-flow Enforcement Technology (CET) Shadow Stack.
  # CET is only supported by x64 and x86 CPUs (11th generation Intel and AMD Zen3 and newer)
  if(${ARM_DETECTED} EQUAL 0)
    add_link_options("/CETCOMPAT")
  endif()

  # Enable LibFuzzer support for MSVC
  if(${ARM_DETECTED} EQUAL 0 AND MSVC_VERSION GREATER_EQUAL 1930)
    set(LIBFUZZER_SUPPORTED 1)
  endif()

endif()

if(CHARLS_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        message(STATUS "Clang-tidy enabled and found at: ${CLANG_TIDY_EXE}")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    else()
        message(WARNING "Clang-tidy enabled, but not found!")
    endif()
endif()

# When enabled apply the pedantic warnings options and warnings as errors to globally.
if(CHARLS_PEDANTIC_WARNINGS)
  if(MSVC)
    # Remove existing warning level (W3 is added by default by CMake), duplicate level will generate cmd-line warnings.
    string(REGEX REPLACE " /W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  endif()

  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:${PEDANTIC_CXX_COMPILE_FLAGS}>")
endif()

if(CHARLS_TREAT_WARNING_AS_ERROR)
  add_compile_options(${WARNINGS_AS_ERRORS_FLAG_COMPILER})
  add_link_options(${WARNINGS_AS_ERRORS_FLAG_LINKER})
endif()

include(src/CMakeLists.txt)

if(CHARLS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)

  # The unit tests project depends on the VS C++ unit test framework. Use the -unittest option of the charlstest tool for other platforms.
  add_test(
    NAME basic-test
    COMMAND charlstest -unittest
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )
endif()

if(CHARLS_BUILD_AFL_FUZZ_TEST)
  add_subdirectory(fuzzing/afl)
endif()

if(CHARLS_BUILD_LIBFUZZER_FUZZ_TEST AND LIBFUZZER_SUPPORTED)
  add_subdirectory(fuzzing/libfuzzer)
endif()

if(CHARLS_BUILD_SAMPLES)
  add_subdirectory(samples)
endif()
