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

target_compile_features(charls_tests PRIVATE cxx_std_17)

if(BUILD_SHARED_LIBS)
    # When charls is built as a DLL, internal symbols are not exported.
    # Compile the charls implementation directly into the test executable to
    # allow white-box access to internal symbols, matching the approach used
    # by the Visual Studio project (linking .obj files directly).
    target_sources(charls_tests PRIVATE
        ../src/charls_jpegls_decoder.cpp
        ../src/charls_jpegls_encoder.cpp
        ../src/golomb_lut.cpp
        ../src/jpeg_stream_reader.cpp
        ../src/jpeg_stream_writer.cpp
        ../src/jpegls_error.cpp
        ../src/make_scan_codec.cpp
        ../src/quantization_lut.cpp
        ../src/validate_spiff_header.cpp
        ../src/version.cpp
    )
    target_include_directories(charls_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    # Use CHARLS_STATIC to avoid __declspec(dllexport) when compiling into an exe.
    target_compile_definitions(charls_tests PRIVATE CHARLS_STATIC)
    target_link_libraries(charls_tests PRIVATE GTest::gtest_main)
else()
    target_link_libraries(charls_tests PRIVATE GTest::gtest_main charls)
endif()

include(GoogleTest)
gtest_discover_tests(charls_tests)
