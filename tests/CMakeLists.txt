# SPDX-FileCopyrightText: © 2026 Team CharLS
# SPDX-License-Identifier: BSD-3-Clause

find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE
    )
    # For Windows: prevent overriding the parent project's compiler/linker settings.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(charls_tests
    color_transform_test.cpp
    golomb_table_test.cpp
    jpeg_error_test.cpp
    jpeg_stream_reader_test.cpp
    jpeg_stream_writer_test.cpp
)

target_include_directories(charls_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_definitions(charls_tests
    PRIVATE CHARLS_LIBRARY_BUILD
)

target_link_libraries(charls_tests
    PRIVATE
        GTest::gtest_main
        charls
)

include(GoogleTest)
gtest_discover_tests(charls_tests)
