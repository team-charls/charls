// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "util.h"

#include "portable_anymap_file.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

using std::cout;
using std::ifstream;
using std::ios;
using std::milli;
using std::ofstream;
using std::setprecision;
using std::setw;
using std::swap;
using std::vector;
using std::chrono::duration;
using std::chrono::steady_clock;
using namespace charls;
using namespace charls_test;


namespace {

MSVC_WARNING_SUPPRESS(26497) // cannot be marked constexpr, check must be executed at runtime.

bool is_machine_little_endian() noexcept
{
    constexpr int a = 0xFF000001; // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
    const auto* chars = reinterpret_cast<const char*>(&a);
    return chars[0] == 0x01;
}

MSVC_WARNING_UNSUPPRESS()

} // namespace


void fix_endian(vector<uint8_t>* buffer, const bool little_endian_data) noexcept
{
    if (little_endian_data == is_machine_little_endian())
        return;

    for (size_t i = 0; i < buffer->size() - 1; i += 2)
    {
        swap((*buffer)[i], (*buffer)[i + 1]);
    }
}


vector<uint8_t> read_file(const char* filename, long offset, size_t bytes)
{
    ifstream input;
    input.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    input.open(filename, ios::in | ios::binary);

    input.seekg(0, ios::end);
    const auto byte_count_file = static_cast<int>(input.tellg());
    input.seekg(offset, ios::beg);

    if (offset < 0)
    {
        assert::is_true(bytes != 0);
        offset = static_cast<long>(byte_count_file - bytes);
    }
    if (bytes == 0)
    {
        bytes = static_cast<size_t>(byte_count_file) - offset;
    }

    vector<uint8_t> buffer(bytes);
    input.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

    return buffer;
}

void write_file(const char* filename, const void* data, const size_t size)
{
    ofstream output;
    output.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    output.open(filename, ios::out | ios::binary);
    output.write(static_cast<const char*>(data), size);
}

void test_round_trip(const char* name, const vector<uint8_t>& decoded_buffer, const rect_size size,
                     const int bits_per_sample, const int component_count, const int loop_count)
{
    JlsParameters params = JlsParameters();
    params.components = component_count;
    params.bitsPerSample = bits_per_sample;
    params.height = static_cast<int>(size.cy);
    params.width = static_cast<int>(size.cx);

    test_round_trip(name, decoded_buffer, params, loop_count);
}


void test_round_trip(const char* name, const vector<uint8_t>& original_buffer, const JlsParameters& params,
                     const int loop_count)
{
    vector<uint8_t> encoded_buffer(params.height * params.width * params.components * params.bitsPerSample / 4);

    vector<uint8_t> decoded_buffer(static_cast<size_t>(params.height) * params.width *
                                   bit_to_byte_count(params.bitsPerSample) * params.components);

    interleave_mode interleave_mode{params.interleaveMode};
    color_transformation color_transformation{params.colorTransformation};

    if (params.components == 4)
    {
        interleave_mode = charls::interleave_mode::line;
    }
    else if (params.components == 3)
    {
        interleave_mode = charls::interleave_mode::line;
        color_transformation = charls::color_transformation::hp1;
    }

    size_t encoded_actual_size{};
    auto start = steady_clock::now();
    for (int i = 0; i < loop_count; ++i)
    {
        try
        {
            jpegls_encoder encoder;
            encoder.destination(encoded_buffer)
                .frame_info({static_cast<uint32_t>(params.width), static_cast<uint32_t>(params.height), params.bitsPerSample,
                             params.components})
                .interleave_mode(interleave_mode)
                .color_transformation(color_transformation);

            encoded_actual_size = encoder.encode(original_buffer);
        }
        catch (...)
        {
            assert::is_true(false);
        }
    }

    const auto total_encode_duration = steady_clock::now() - start;

    start = steady_clock::now();
    for (int i = 0; i < loop_count; ++i)
    {
        try
        {
            jpegls_decoder decoder;
            decoder.source(encoded_buffer.data(), encoded_actual_size).read_header();
            decoder.decode(decoded_buffer);
        }
        catch (...)
        {
            assert::is_true(false);
        }
    }

    const auto total_decode_duration = steady_clock::now() - start;

    const double bits_per_sample = 1.0 * static_cast<double>(encoded_actual_size) * 8. /
                                   (static_cast<double>(params.components) * params.height * params.width);
    cout << "RoundTrip test for: " << name << "\n\r";
    const double encode_time = duration<double, milli>(total_encode_duration).count() / loop_count;
    const double decode_time = duration<double, milli>(total_decode_duration).count() / loop_count;
    const double symbol_rate =
        (static_cast<double>(params.components) * params.height * params.width) / (1000.0 * decode_time);

    cout << "Size:" << setw(10) << params.width << "x" << params.height << setw(7) << setprecision(2)
         << ", Encode time:" << encode_time << " ms, Decode time:" << decode_time
         << " ms, Bits per sample:" << bits_per_sample << ", Decode rate:" << symbol_rate << " M/s\n";

    const uint8_t* byte_out = decoded_buffer.data();
    for (size_t i = 0; i < decoded_buffer.size(); ++i)
    {
        if (original_buffer[i] != byte_out[i])
        {
            assert::is_true(false);
            break;
        }
    }
}


void test_file(const char* filename, const int offset, const rect_size size2, const int bits_per_sample,
               const int component_count, const bool little_endian_file, const int loop_count)
{
    const size_t byte_count = size2.cx * size2.cy * component_count * bit_to_byte_count(bits_per_sample);
    vector<uint8_t> uncompressed_buffer = read_file(filename, offset, byte_count);

    if (bits_per_sample > 8)
    {
        fix_endian(&uncompressed_buffer, little_endian_file);
    }

    test_round_trip(filename, uncompressed_buffer, size2, bits_per_sample, component_count, loop_count);
}


void test_portable_anymap_file(const char* filename, const int loop_count)
{
    portable_anymap_file anymap_file(filename);

    test_round_trip(filename, anymap_file.image_data(), rect_size(anymap_file.width(), anymap_file.height()),
                    anymap_file.bits_per_sample(), anymap_file.component_count(), loop_count);
}
