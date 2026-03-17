# SPDX-FileCopyrightText: © 2017 Team CharLS
# SPDX-License-Identifier: BSD-3-Clause

find_package(GTest REQUIRED)

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
