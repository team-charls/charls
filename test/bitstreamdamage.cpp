// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "bitstreamdamage.h"
#include "util.h"

#include <iostream>
#include <random>
#include <vector>

using charls::jpegls_decoder;
using charls::jpegls_errc;
using charls::jpegls_error;
using std::cout;
using std::error_code;
using std::mt19937;
using std::uniform_int_distribution;
using std::vector;

namespace {

void test_damaged_bit_stream1()
{
    const vector<uint8_t> encoded_buffer = read_file("test/incorrect_images/InfiniteLoopFFMPEG.jls");
    vector<uint8_t> destination(256 * 256 * 2);

    error_code error;
    try
    {
        jpegls_decoder::decode(encoded_buffer, destination);
    }
    catch (const jpegls_error& e)
    {
        error = e.code();
    }

    assert::is_true(error == jpegls_errc::invalid_encoded_data);
}


void test_damaged_bit_stream2()
{
    vector<uint8_t> encoded_buffer = read_file("test/lena8b.jls");

    encoded_buffer.resize(900);
    encoded_buffer.resize(40000, 3);

    vector<uint8_t> destination(512 * 512);

    error_code error;
    try
    {
        jpegls_decoder::decode(encoded_buffer, destination);
    }
    catch (const jpegls_error& e)
    {
        error = e.code();
    }

    assert::is_true(error == jpegls_errc::invalid_encoded_data);
}


void test_damaged_bit_stream3()
{
    vector<uint8_t> encoded_buffer = read_file("test/lena8b.jls");

    encoded_buffer[300] = 0xFF;
    encoded_buffer[301] = 0xFF;

    vector<uint8_t> destination(512 * 512);

    error_code error;
    try
    {
        jpegls_decoder::decode(encoded_buffer, destination);
    }
    catch (const jpegls_error& e)
    {
        error = e.code();
    }

    assert::is_true(error == jpegls_errc::invalid_encoded_data);
}


void test_file_with_random_header_damage(const char* filename)
{
    const vector<uint8_t> encoded_buffer_original = read_file(filename);

    mt19937 generator(102347325);

    MSVC_WARNING_SUPPRESS_NEXT_LINE(26496) // cannot be marked as const as operator() is not always defined const.
    MSVC_CONST uniform_int_distribution<uint32_t> distribution(0, 255);

    vector<uint8_t> destination(512 * 512);

    for (size_t i = 0; i < 40; ++i)
    {
        vector<uint8_t> encoded_buffer(encoded_buffer_original);
        vector<int> errors(10, 0);

        for (int j = 0; j < 20; ++j)
        {
            encoded_buffer[i] = static_cast<uint8_t>(distribution(generator));
            encoded_buffer[i + 1] = static_cast<uint8_t>(distribution(generator));
            encoded_buffer[i + 2] = static_cast<uint8_t>(distribution(generator));
            encoded_buffer[i + 3] = static_cast<uint8_t>(distribution(generator));

            error_code error;
            try
            {
                jpegls_decoder::decode(encoded_buffer, destination);
            }
            catch (const jpegls_error& e)
            {
                error = e.code();
            }
            errors[static_cast<size_t>(error.value())]++;
        }

        cout << "With garbage input at index " << i << ": ";
        for (unsigned int error = 0; error < errors.size(); ++error)
        {
            if (errors[error] == 0)
                continue;

            cout << errors[error] << "x error (" << error << "); ";
        }

        cout << "\r\n";
    }
}


void test_random_malformed_header()
{
    test_file_with_random_header_damage("test/conformance/t8c0e0.jls");
    test_file_with_random_header_damage("test/conformance/t8c1e0.jls");
    test_file_with_random_header_damage("test/conformance/t8c2e0.jls");
}


} // namespace


void damaged_bit_stream_tests()
{
    cout << "Test Damaged bit stream\r\n";
    test_damaged_bit_stream1();
    test_damaged_bit_stream2();
    test_damaged_bit_stream3();

    cout << "Begin random malformed bit stream tests:\n";
    test_random_malformed_header();
    cout << "End random malformed bit stream tests:\n";
}
