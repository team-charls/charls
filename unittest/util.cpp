// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include "../src/jpeg_stream_writer.h"

#include "../test/portable_anymap_file.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::ifstream;
using std::vector;
using namespace charls;
using namespace charls_test;

namespace {

void triplet_to_planar(vector<uint8_t>& buffer, uint32_t width, uint32_t height)
{
    vector<uint8_t> workBuffer(buffer.size());

    const size_t byteCount = static_cast<size_t>(width) * height;
    for (size_t index = 0; index < byteCount; index++)
    {
        workBuffer[index] = buffer[index * 3 + 0];
        workBuffer[index + 1 * byteCount] = buffer[index * 3 + 1];
        workBuffer[index + 2 * byteCount] = buffer[index * 3 + 2];
    }
    swap(buffer, workBuffer);
}

} // namespace

vector<uint8_t> read_file(const char* filename)
{
    ifstream input;
    input.exceptions(input.eofbit | input.failbit | input.badbit);
    input.open(filename, input.in | input.binary);

    input.seekg(0, input.end);
    const auto byteCountFile = static_cast<int>(input.tellg());
    input.seekg(0, input.beg);

    vector<uint8_t> buffer(byteCountFile);
    input.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

    return buffer;
}

portable_anymap_file read_anymap_reference_file(const char* filename, const interleave_mode interleave_mode, const frame_info& frame_info)
{
    portable_anymap_file reference_file(filename);

    if (interleave_mode == interleave_mode::none && frame_info.component_count == 3)
    {
        triplet_to_planar(reference_file.image_data(), frame_info.width, frame_info.height);
    }

    return reference_file;
}

portable_anymap_file read_anymap_reference_file(const char* filename, const interleave_mode interleave_mode)
{
    portable_anymap_file reference_file(filename);

    if (interleave_mode == interleave_mode::none && reference_file.component_count() == 3)
    {
        triplet_to_planar(reference_file.image_data(), reference_file.width(), reference_file.height());
    }

    return reference_file;
}

vector<uint8_t> create_test_spiff_header(const uint8_t high_version, const uint8_t low_version, const bool end_of_directory)
{
    vector<uint8_t> buffer;
    buffer.push_back(0xFF);
    buffer.push_back(0xD8); // SOI.
    buffer.push_back(0xFF);
    buffer.push_back(0xE8); // ApplicationData8
    buffer.push_back(0);
    buffer.push_back(32);

    // SPIFF identifier string.
    buffer.push_back('S');
    buffer.push_back('P');
    buffer.push_back('I');
    buffer.push_back('F');
    buffer.push_back('F');
    buffer.push_back(0);

    // Version
    buffer.push_back(high_version);
    buffer.push_back(low_version);

    buffer.push_back(0); // profile id
    buffer.push_back(3); // component count

    // Height
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0x3);
    buffer.push_back(0x20);

    // Width
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0x2);
    buffer.push_back(0x58);

    buffer.push_back(10); // color space
    buffer.push_back(8);  // bits per sample
    buffer.push_back(6);  // compression type, 6 = JPEG-LS
    buffer.push_back(1);  // resolution units

    // vertical_resolution
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(96);

    // header.horizontal_resolution = 1024;
    buffer.push_back(0);
    buffer.push_back(0);
    buffer.push_back(4);
    buffer.push_back(0);

    const size_t spiff_header_size = buffer.size();
    buffer.resize(buffer.size() + 100);

    const ByteStreamInfo info = FromByteArray(buffer.data() + spiff_header_size, buffer.size() - spiff_header_size);
    JpegStreamWriter writer(info);

    if (end_of_directory)
    {
        writer.WriteSpiffEndOfDirectoryEntry();
    }

    writer.WriteStartOfFrameSegment(100, 100, 8, 1);
    writer.WriteStartOfScanSegment(1, 0, interleave_mode::none);

    return buffer;
}

vector<uint8_t> create_noise_image_16bit(const size_t pixel_count, const int bit_count, const uint32_t seed)
{
    srand(seed);
    vector<uint8_t> buffer(pixel_count * 2);
    const auto mask = static_cast<uint16_t>((1 << bit_count) - 1);
    for (size_t i = 0; i < pixel_count; i = i + 2)
    {
        const uint16_t value = static_cast<uint16_t>(rand()) & mask;

        buffer[i] = static_cast<uint8_t>(value);
        buffer[i] = static_cast<uint8_t>(value >> 8);
    }
    return buffer;
}

void test_round_trip_legacy(const vector<uint8_t>& source, const JlsParameters& params)
{
    vector<uint8_t> encodedBuffer(params.height * params.width * params.components * params.bitsPerSample / 4);
    vector<uint8_t> decodedBuffer(static_cast<size_t>(params.height) * params.width * ((params.bitsPerSample + 7) / 8) * params.components);

    size_t compressedLength = 0;
    auto error = JpegLsEncode(encodedBuffer.data(), encodedBuffer.size(), &compressedLength,
                              source.data(), source.size(), &params, nullptr);
    Assert::AreEqual(jpegls_errc::success, error);

    error = JpegLsDecode(decodedBuffer.data(), decodedBuffer.size(), encodedBuffer.data(), compressedLength, nullptr, nullptr);
    Assert::AreEqual(jpegls_errc::success, error);

    const uint8_t* byteOut = decodedBuffer.data();
    for (size_t i = 0; i < decodedBuffer.size(); ++i)
    {
        if (source[i] != byteOut[i])
        {
            Assert::IsTrue(false);
            break;
        }
    }
}
