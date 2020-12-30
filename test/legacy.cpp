// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "legacy.h"

#include "util.h"

#include <array>
#include <iostream>
#include <vector>

using std::array;
using std::cout;
using std::vector;
using namespace charls;

namespace {

// ReSharper disable CppDeprecatedEntity
DISABLE_DEPRECATED_WARNING

void test_jpegls_read_header(const char* filename, const int width, const int height, const int bits_per_sample,
                             const int stride, const int component_count, const int interleave_mode)
{
    cout << "LegacyAPI JpegLsReadHeader:" << filename << "\n";

    vector<uint8_t> encoded_buffer = read_file(filename);

    array<char, ErrorMessageSize> error_message{};
    JlsParameters parameters{};
    const auto error = JpegLsReadHeader(encoded_buffer.data(), encoded_buffer.size(), &parameters, error_message.data());
    assert::is_true(error == jpegls_errc::success);

    assert::is_true(parameters.width == width || parameters.height == height ||
                    parameters.bitsPerSample == bits_per_sample || parameters.stride == stride ||
                    parameters.components == component_count ||
                    parameters.interleaveMode == static_cast<charls::interleave_mode>(interleave_mode));
}

// ReSharper restore CppDeprecatedEntity
RESTORE_DEPRECATED_WARNING

void test_jpegls_read_header()
{
    cout << "Test JpegLsReadHeader\n";

    test_jpegls_read_header("test/conformance/t8c0e0.jls", 256, 256, 8, 256, 3, 0);
    test_jpegls_read_header("test/conformance/t8c1e0.jls", 256, 256, 8, 768, 3, 1);
    test_jpegls_read_header("test/conformance/t8c2e0.jls", 256, 256, 8, 768, 3, 2);
    test_jpegls_read_header("test/conformance/t8c0e3.jls", 256, 256, 8, 256, 3, 0);
    test_jpegls_read_header("test/conformance/t8c1e3.jls", 256, 256, 8, 768, 3, 1);
    test_jpegls_read_header("test/conformance/t8c2e3.jls", 256, 256, 8, 768, 3, 2);
    test_jpegls_read_header("test/conformance/t8nde0.jls", 128, 128, 8, 128, 1, 0);
    test_jpegls_read_header("test/conformance/t8nde3.jls", 128, 128, 8, 128, 1, 0);
    test_jpegls_read_header("test/conformance/t16e0.jls", 256, 256, 12, 512, 1, 0);
    test_jpegls_read_header("test/conformance/t16e3.jls", 256, 256, 12, 512, 1, 0);
    test_jpegls_read_header("test/lena8b.jls", 512, 512, 8, 512, 1, 0);
}

} // namespace

void test_legacy_api()
{
    cout << "Test LegacyAPIs\n";

    test_jpegls_read_header();
}
