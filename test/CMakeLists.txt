# SPDX-FileCopyrightText: © 2026 Team CharLS
# SPDX-License-Identifier: BSD-3-Clause

find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG 94be250af7e14c58dcbf476972d2d7141551ff67 # main, Mar 10, 2026
    )
    # For Windows: prevent overriding the parent project's compiler/linker settings.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # Always build googletest as a static library so that the test executable is
    # self-contained and does not require gtest DLLs to be in PATH at runtime.
    set(BUILD_SHARED_LIBS_SAVED ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(googletest)
    set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_SAVED})
    # Suppress all warnings on the fetched googletest targets so that project-wide
    # pedantic warning flags (e.g. -Werror) do not fail third-party sources.
    # Note: with CMake 3.25+ this can be solved more cleanly by passing SYSTEM to
    # FetchContent_Declare, which marks the fetched targets as system libraries and
    # automatically suppresses warnings on their include directories.
    foreach(target IN ITEMS gtest gtest_main gmock gmock_main)
        if(TARGET ${target})
            target_compile_options(${target} PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-w>
                $<$<CXX_COMPILER_ID:MSVC>:/w>
            )
        endif()
    endforeach()
endif()

add_executable(charls-test
    charls_jpegls_decoder_test.cpp
    charls_jpegls_encoder_test.cpp
    color_transform_test.cpp
    compliance_test.cpp
    default_traits_test.cpp
    documentation_test.cpp
    encode_test.cpp
    golomb_table_test.cpp
    jpeg_error_test.cpp
    jpeg_stream_reader_test.cpp
    jpeg_stream_writer_test.cpp
    jpegls_algorithm_test.cpp
    jpegls_decoder_test.cpp
    jpegls_encoder_test.cpp
    jpegls_preset_coding_parameters_test.cpp
    lossless_traits_test.cpp
    quantization_lut_test.cpp
    regular_mode_context_test.cpp
    run_mode_context_test.cpp
    sample_traits_test.cpp
    scan_decoder_test.cpp
    scan_encoder_test.cpp
    support.cpp
    util_test.cpp
    validate_spiff_header_test.cpp
    version_test.cpp
)

target_precompile_headers(charls-test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/pch.hpp)

target_include_directories(charls-test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_features(charls-test PRIVATE cxx_std_17)

if(BUILD_SHARED_LIBS)
    # When charls is built as a DLL, internal symbols are not exported.
    # Compile the charls implementation directly into the test executable to
    # allow white-box access to internal symbols, matching the approach used
    # by the Visual Studio project (linking .obj files directly).
    target_sources(charls-test PRIVATE
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
    target_include_directories(charls-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    # Use CHARLS_STATIC to avoid __declspec(dllexport) when compiling into an exe.
    target_compile_definitions(charls-test PRIVATE CHARLS_STATIC)
    target_link_libraries(charls-test PRIVATE GTest::gtest_main)
else()
    target_link_libraries(charls-test PRIVATE GTest::gtest_main charls)
endif()

include(GoogleTest)
# Use PRE_TEST discovery so the test executable is not invoked at build time.
# This avoids failures when cross-compiling (e.g. building ARM64 on an x64 host).
gtest_discover_tests(charls-test DISCOVERY_MODE PRE_TEST)

# Copy test data files to a data sub-directory next to the test
# executable at build time. The source files come from the repository and are not
# duplicated; the copies exist only in the build tree.
set(CHARLS_TEST_DATA_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/t8c0e0.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/t8c1e0.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/t8c2e0.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/t8c0e3.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/t8c1e3.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/t8c2e3.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/t8nde0.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/t8nde3.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/t16e0.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/t16e3.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/test8.ppm
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/test8bs2.pgm
    ${CMAKE_CURRENT_SOURCE_DIR}/data/conformance/test16.pgm
    ${CMAKE_CURRENT_SOURCE_DIR}/data/tulips-gray-8bit-512-512-hp-encoder.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/16-bit-640-480-many-dots.pgm
    ${CMAKE_CURRENT_SOURCE_DIR}/data/2bit_parrot_150x200.pgm
    ${CMAKE_CURRENT_SOURCE_DIR}/data/4bit-monochrome.pgm
    ${CMAKE_CURRENT_SOURCE_DIR}/data/tulips-gray-8bit-512-512.pgm
    ${CMAKE_CURRENT_SOURCE_DIR}/data/test8_ilv_none_rm_7.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/test8_ilv_line_rm_7.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/test8_ilv_sample_rm_7.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/test8_ilv_sample_rm_300.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/test16_rm_5.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/8bit-monochrome-2x2.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/ff_in_entropy_data.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/fuzzy_input_golomb_16.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/fuzzy-input-bad-run-mode-golomb-code.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/fuzzy-input-no-valid-bits-at-the-end.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/land10-10bit-rgb-hp3-invalid.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/no_start_byte_after_encoded_scan.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/banny.ppm
    ${CMAKE_CURRENT_SOURCE_DIR}/data/banny.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/banny-hp1.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/banny-hp2.jls
    ${CMAKE_CURRENT_SOURCE_DIR}/data/banny-hp3.jls
)

add_custom_command(TARGET charls-test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:charls-test>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CHARLS_TEST_DATA_FILES}
        "$<TARGET_FILE_DIR:charls-test>/data/"
    COMMENT "Copying test data files to build directory"
)
