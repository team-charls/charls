// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "../src/default_traits.h"
#include "../src/lossless_traits.h"
#include "../src/process_line.h"

#include "util.h"

#include "bitstreamdamage.h"
#include "compliance.h"
#include "dicomsamples.h"
#include "legacy.h"
#include "performance.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <vector>

using std::array;
using std::cout;
using std::error_code;
using std::getline;
using std::ifstream;
using std::ios;
using std::istream;
using std::iter_swap;
using std::mt19937;
using std::ofstream;
using std::ostream;
using std::streamoff;
using std::string;
using std::stringstream;
using std::runtime_error;
using std::uniform_int_distribution;
using std::vector;
using namespace charls;

namespace {

constexpr ios::openmode mode_input{ios::in | ios::binary};

ifstream open_input_stream(const char* filename)
{
    ifstream stream;
    stream.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    stream.open(filename, mode_input);

    return stream;
}


uint32_t log2_floor(const uint32_t n) noexcept
{
    ASSERT(n != 0 && "log2 is not defined for 0");
    return 31 - countl_zero(n);
}


constexpr int result_to_exit_code(const bool result) noexcept
{
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}


uint32_t max_value_to_bits_per_sample(const uint32_t max_value) noexcept
{
    ASSERT(max_value > 0);
    return log2_floor(max_value) + 1;
}


template<typename Container>
void read(istream& input, Container& destination_container)
{
    input.read(reinterpret_cast<char*>(destination_container.data()), destination_container.size());
}


size_t get_stream_length(istream& stream, const size_t end_offset = 0)
{
    stream.seekg(0, ios::end);
    const auto length{static_cast<size_t>(stream.tellg())};
    stream.seekg(static_cast<ios::off_type>(end_offset), ios::beg);

    return length;
}


template<typename SizeType>
void convert_planar_to_pixel(const size_t width, const size_t height, const void* source, void* destination) noexcept
{
    const size_t stride_in_pixels = width * 3;
    const auto* plane0{static_cast<const SizeType*>(source)};
    const auto* plane1{static_cast<const SizeType*>(source) + (width * height)};
    const auto* plane2{static_cast<const SizeType*>(source) + (width * height * 2)};

    auto* pixels{static_cast<SizeType*>(destination)};

    for (size_t row{}; row != height; ++row)
    {
        for (size_t column{}, offset = 0; column != width; ++column, offset += 3)
        {
            pixels[offset + 0] = plane0[column];
            pixels[offset + 1] = plane1[column];
            pixels[offset + 2] = plane2[column];
        }

        plane0 += width;
        plane1 += width;
        plane2 += width;
        pixels += stride_in_pixels;
    }
}


void test_traits16_bit()
{
    const auto traits1 = default_traits<uint16_t, uint16_t>(4095, 0);
    using lossless_traits = lossless_traits<uint16_t, 12>;

    assert::is_true(traits1.limit == lossless_traits::limit);
    assert::is_true(traits1.maximum_sample_value == lossless_traits::maximum_sample_value);
    assert::is_true(traits1.reset_threshold == lossless_traits::reset_threshold);
    assert::is_true(traits1.bits_per_pixel == lossless_traits::bits_per_pixel);
    assert::is_true(traits1.quantized_bits_per_pixel == lossless_traits::quantized_bits_per_pixel);

    for (int i{-4096}; i != 4096; ++i)
    {
        assert::is_true(traits1.modulo_range(i) == lossless_traits::modulo_range(i));
        assert::is_true(traits1.compute_error_value(i) == lossless_traits::compute_error_value(i));
    }

    for (int i{-8095}; i != 8095; ++i)
    {
        assert::is_true(traits1.correct_prediction(i) == lossless_traits::correct_prediction(i));
        assert::is_true(traits1.is_near(i, 2) == lossless_traits::is_near(i, 2));
    }
}


void test_traits8_bit()
{
    const auto traits1{default_traits<uint8_t, uint8_t>(255, 0)};
    using lossless_traits = lossless_traits<uint8_t, 8>;

    assert::is_true(traits1.limit == lossless_traits::limit);
    assert::is_true(traits1.maximum_sample_value == lossless_traits::maximum_sample_value);
    assert::is_true(traits1.reset_threshold == lossless_traits::reset_threshold);
    assert::is_true(traits1.bits_per_pixel == lossless_traits::bits_per_pixel);
    assert::is_true(traits1.quantized_bits_per_pixel == lossless_traits::quantized_bits_per_pixel);

    for (int i{-255}; i != 255; ++i)
    {
        assert::is_true(traits1.modulo_range(i) == lossless_traits::modulo_range(i));
        assert::is_true(traits1.compute_error_value(i) == lossless_traits::compute_error_value(i));
    }

    for (int i{-255}; i != 512; ++i)
    {
        assert::is_true(traits1.correct_prediction(i) == lossless_traits::correct_prediction(i));
        assert::is_true(traits1.is_near(i, 2) == lossless_traits::is_near(i, 2));
    }
}


vector<uint8_t> make_some_noise(const size_t length, const size_t bit_count, const int seed)
{
    const auto max_value{(1U << bit_count) - 1U};
    mt19937 generator(seed);

    MSVC_WARNING_SUPPRESS_NEXT_LINE(26496) // cannot be marked as const as operator() is not always defined const.
    uniform_int_distribution<uint32_t> distribution(0, max_value);

    vector<uint8_t> buffer(length);
    for (auto& pixel_value : buffer)
    {
        pixel_value = static_cast<uint8_t>(distribution(generator));
    }

    return buffer;
}


vector<uint8_t> make_some_noise16_bit(const size_t length, const int bit_count, const int seed)
{
    const auto max_value{static_cast<uint16_t>((1U << bit_count) - 1U)};
    mt19937 generator(seed);

    MSVC_WARNING_SUPPRESS_NEXT_LINE(26496) // cannot be marked as const as operator() is not always defined const.
    uniform_int_distribution<uint16_t> distribution{0, max_value};

    vector<uint8_t> buffer(length * 2);
    for (size_t i{}; i != length; i = i + 2)
    {
        const uint16_t value{distribution(generator)};

        buffer[i] = static_cast<uint8_t>(value);
        buffer[i] = static_cast<uint8_t>(value >> 8);
    }

    return buffer;
}


void test_noise_image()
{
    const rect_size size2{512, 512};

    for (size_t bit_depth{8}; bit_depth >= 2; --bit_depth)
    {
        stringstream label;
        label << "noise, bit depth: " << bit_depth;

        const vector<uint8_t> noise_bytes = make_some_noise(size2.cx * size2.cy, bit_depth, 21344);
        test_round_trip(label.str().c_str(), noise_bytes, size2, static_cast<int>(bit_depth), 1);
    }

    for (int bit_depth{16}; bit_depth > 8; --bit_depth)
    {
        stringstream label;
        label << "noise, bit depth: " << bit_depth;

        const vector<uint8_t> noise_bytes = make_some_noise16_bit(size2.cx * size2.cy, bit_depth, 21344);
        test_round_trip(label.str().c_str(), noise_bytes, size2, bit_depth, 1);
    }
}


void test_noise_image_with_custom_reset()
{
    const rect_size size{512, 512};
    constexpr int bit_depth{16};
    const vector<uint8_t> noise_bytes = make_some_noise16_bit(size.cx * size.cy, bit_depth, 21344);

    JlsParameters params{};
    params.components = 1;
    params.bitsPerSample = bit_depth;
    params.height = static_cast<int>(size.cy);
    params.width = static_cast<int>(size.cx);
    params.custom.MaximumSampleValue = (1 << bit_depth) - 1;
    params.custom.ResetValue = 63;

    test_round_trip("TestNoiseImageWithCustomReset", noise_bytes, params);
}


void test_fail_on_too_small_output_buffer()
{
    const auto input_buffer{make_some_noise(static_cast<size_t>(8) * 8, 8, 21344)};

    // Trigger a "destination buffer too small" when writing the header markers.
    try
    {
        vector<uint8_t> output_buffer(1);
        jpegls_encoder encoder;
        encoder.destination(output_buffer);
        encoder.frame_info({8, 8, 8, 1});
        std::ignore = encoder.encode(input_buffer);
        assert::is_true(false);
    }
    catch (const jpegls_error& e)
    {
        assert::is_true(e.code() == jpegls_errc::destination_buffer_too_small);
    }

    // Trigger a "destination buffer too small" when writing the encoded pixel bytes.
    try
    {
        vector<uint8_t> output_buffer(100);
        jpegls_encoder encoder;
        encoder.destination(output_buffer);
        encoder.frame_info({8, 8, 8, 1});
        std::ignore = encoder.encode(input_buffer);
        assert::is_true(false);
    }
    catch (const jpegls_error& e)
    {
        assert::is_true(e.code() == jpegls_errc::destination_buffer_too_small);
    }
}


void test_bgra()
{
    array<uint8_t, 20> input{'R', 'G', 'B', 'A', 'R', 'G', 'B', 'A', 'R', 'G', 'B', 'A', 'R', 'G', 'B', 'A', 1, 2, 3, 4};
    const array<uint8_t, 20> expected{'B', 'G', 'R', 'A', 'B', 'G', 'R', 'A', 'B', 'G',
                                      'R', 'A', 'B', 'G', 'R', 'A', 1,   2,   3,   4};

    transform_rgb_to_bgr(input.data(), 4, 4);
    assert::is_true(expected == input);
}


void test_bgr()
{
    const vector<uint8_t> encoded_source{read_file("test/conformance/t8c2e3.jls")};

    jpegls_decoder decoder;
    decoder.source(encoded_source);
    decoder.read_header();

    vector<uint8_t> decoded_buffer(decoder.destination_size());

    // ReSharper disable CppDeprecatedEntity
    DISABLE_DEPRECATED_WARNING

    JlsParameters params{};
    params.outputBgr = static_cast<char>(true);

    const error_code error = JpegLsDecode(decoded_buffer.data(), decoded_buffer.size(), encoded_source.data(),
                                          encoded_source.size(), &params, nullptr);

    // ReSharper restore CppDeprecatedEntity
    RESTORE_DEPRECATED_WARNING

    assert::is_true(!error);

    assert::is_true(decoded_buffer[0] == 0x69);
    assert::is_true(decoded_buffer[1] == 0x77);
    assert::is_true(decoded_buffer[2] == 0xa1);
    assert::is_true(decoded_buffer[static_cast<size_t>(decoder.frame_info().width) * 6 + 3] == 0x2d);
    assert::is_true(decoded_buffer[static_cast<size_t>(decoder.frame_info().width) * 6 + 4] == 0x43);
    assert::is_true(decoded_buffer[static_cast<size_t>(decoder.frame_info().width) * 6 + 5] == 0x4d);
}


void test_too_small_output_buffer()
{
    const vector<uint8_t> encoded{read_file("test/tulips-gray-8bit-512-512-hp-encoder.jls")};
    vector<uint8_t> destination(size_t{512} * 511);

    jpegls_decoder decoder;
    decoder.source(encoded).read_header();

    error_code error;
    try
    {
        decoder.decode(destination);
    }
    catch (const jpegls_error& e)
    {
        error = e.code();
    }

    assert::is_true(error == jpegls_errc::destination_buffer_too_small);
}


void test_decode_bit_stream_with_no_marker_start()
{
    const array<uint8_t, 2> encoded_data{0x33, 0x33};
    array<uint8_t, 1000> output{};

    error_code error;
    try
    {
        jpegls_decoder decoder;
        decoder.source(encoded_data).read_header();
        decoder.decode(output);
    }
    catch (const jpegls_error& e)
    {
        error = e.code();
    }

    assert::is_true(error == jpegls_errc::jpeg_marker_start_byte_not_found);
}


void test_decode_bit_stream_with_unsupported_encoding()
{
    const array<uint8_t, 6> encoded_data{
        0xFF, 0xD8, // Start Of Image (JPEG_SOI)
        0xFF, 0xC3, // Start Of Frame (lossless, Huffman) (JPEG_SOF_3)
        0x00, 0x00  // Length of data of the marker
    };
    array<uint8_t, 1000> output{};

    error_code error;
    try
    {
        jpegls_decoder decoder;
        decoder.source(encoded_data).read_header();
        decoder.decode(output);
    }
    catch (const jpegls_error& e)
    {
        error = e.code();
    }

    assert::is_true(error == jpegls_errc::encoding_not_supported);
}


void test_decode_bit_stream_with_unknown_jpeg_marker()
{
    const array<uint8_t, 6> encoded_data{
        0xFF, 0xD8, // Start Of Image (JPEG_SOI)
        0xFF, 0x01, // Undefined marker
        0x00, 0x00  // Length of data of the marker
    };
    array<uint8_t, 1000> output{};

    error_code error;
    try
    {
        jpegls_decoder decoder;
        decoder.source(encoded_data).read_header();
        decoder.decode(output);
    }
    catch (const jpegls_error& e)
    {
        error = e.code();
    }

    assert::is_true(error == jpegls_errc::unknown_jpeg_marker_found);
}


void test_decode_rect()
{
    const vector<uint8_t> encoded_source{read_file("test/tulips-gray-8bit-512-512-hp-encoder.jls")};

    const jpegls_decoder decoder{encoded_source, true};
    vector<uint8_t> decoded_buffer(decoder.destination_size());

    // ReSharper disable CppDeprecatedEntity
    DISABLE_DEPRECATED_WARNING

    constexpr JlsParameters params{};

    error_code error = JpegLsDecode(decoded_buffer.data(), decoded_buffer.size(), encoded_source.data(),
                                    encoded_source.size(), &params, nullptr);
    assert::is_true(!error);

    constexpr JlsRect rect{128, 128, 256, 1};
    vector<uint8_t> decoded_data(static_cast<size_t>(rect.Width) * rect.Height);
    decoded_data.push_back(0x1f);

    error = JpegLsDecodeRect(decoded_data.data(), decoded_data.size(), encoded_source.data(), encoded_source.size(), rect,
                             nullptr, nullptr);

    // ReSharper restore CppDeprecatedEntity
    RESTORE_DEPRECATED_WARNING

    assert::is_true(!error);

    assert::is_true(memcmp(&decoded_buffer[rect.X + static_cast<size_t>(rect.Y) * 512], decoded_data.data(),
                           static_cast<size_t>(rect.Width) * rect.Height) == 0);
    assert::is_true(decoded_data[static_cast<size_t>(rect.Width) * rect.Height] == 0x1f);
}


void test_encode_from_stream(const char* filename, const size_t offset, const uint32_t width, const uint32_t height,
                             const int32_t bits_per_sample, const int32_t component_count,
                             const interleave_mode interleave_mode, const size_t expected_length)
{
    ifstream source_file{open_input_stream(filename)};

    size_t length{get_stream_length(source_file, offset)};
    assert::is_true(length >= offset);
    length -= offset;

    // Note: use a buffer until the new API provides passing a callback function to read.
    vector<uint8_t> source(length);
    read(source_file, source);

    jpegls_encoder encoder;
    encoder.frame_info({width, height, bits_per_sample, component_count}).interleave_mode(interleave_mode);

    vector<uint8_t> encoded_destination(encoder.estimated_destination_size());
    encoder.destination(encoded_destination);

    assert::is_true(encoder.encode(source) == expected_length);
}


bool decode_to_pnm(const char* filename_input, const char* filename_output)
try
{
    ifstream input{open_input_stream(filename_input)};

    const size_t length{get_stream_length(input)};
    vector<uint8_t> encoded_source(length);
    read(input, encoded_source);

    vector<uint8_t> decoded_destination;
    frame_info frame_info;
    interleave_mode interleave_mode;
    std::tie(frame_info, interleave_mode) = jpegls_decoder::decode(encoded_source, decoded_destination);

    if (frame_info.component_count != 1 && frame_info.component_count != 3)
    {
        cout << "Only JPEG-LS images with component count 1 or 3 are supported to decode to pnm\n";
        return false;
    }

    // PPM format only supports by-pixel, convert if needed.
    if (interleave_mode == charls::interleave_mode::none && frame_info.component_count == 3)
    {
        vector<uint8_t> pixels(decoded_destination.size());
        if (frame_info.bits_per_sample > 8)
        {
            convert_planar_to_pixel<uint16_t>(frame_info.width, frame_info.height, decoded_destination.data(),
                                              pixels.data());
        }
        else
        {
            convert_planar_to_pixel<uint8_t>(frame_info.width, frame_info.height, decoded_destination.data(), pixels.data());
        }

        swap(decoded_destination, pixels);
    }

    // PNM format requires most significant byte first (big endian).
    const int max_value = (1 << frame_info.bits_per_sample) - 1;
    const int bytes_per_sample = max_value > 255 ? 2 : 1;

    if (bytes_per_sample == 2)
    {
        for (auto i = decoded_destination.begin(); i != decoded_destination.end(); i += 2)
        {
            iter_swap(i, i + 1);
        }
    }

    const int magic_number{frame_info.component_count == 3 ? 6 : 5};

    ofstream output{open_output_stream(filename_output)};
    output << 'P' << magic_number << "\n" << frame_info.width << ' ' << frame_info.height << "\n" << max_value << "\n";
    write(output, decoded_destination, decoded_destination.size());
    output.close();

    return true;
}
catch (const runtime_error& error)
{
    cout << "Failed to decode " << filename_input << " to " << filename_output << ", reason: " << error.what() << '\n';
    return false;
}


vector<int> read_pnm_header(istream& pnm_file)
{
    vector<int> read_values;

    const auto first = static_cast<char>(pnm_file.get());

    // All portable anymap format (PNM) start with the character P.
    if (first != 'P')
        return read_values;

    while (read_values.size() < 4)
    {
        string bytes;
        getline(pnm_file, bytes);
        stringstream line(bytes);

        while (read_values.size() < 4)
        {
            int value = -1;
            line >> value;
            if (value <= 0)
                break;

            read_values.push_back(value);
        }
    }
    return read_values;
}


// Purpose: this function can encode an image stored in the Portable Anymap Format (PNM)
//          into the JPEG-LS format. The 2 binary formats P5 and P6 are supported:
//          Portable GrayMap: P5 = binary, extension = .pgm, 0-2^16 (gray scale)
//          Portable PixMap: P6 = binary, extension.ppm, range 0-2^16 (RGB)
bool encode_pnm(const char* filename_input, const char* filename_output)
try
{
    ifstream pnm_file(open_input_stream(filename_input));

    const vector<int> read_values{read_pnm_header(pnm_file)};
    if (read_values.size() != 4)
        return false;

    const frame_info frame_info{static_cast<uint32_t>(read_values[1]), static_cast<uint32_t>(read_values[2]),
                                static_cast<int32_t>(max_value_to_bits_per_sample(read_values[3])),
                                read_values[0] == 6 ? 3 : 1};

    const auto bytes_per_sample = static_cast<int32_t>(::bit_to_byte_count(frame_info.bits_per_sample));
    vector<uint8_t> input_buffer(static_cast<size_t>(frame_info.width) * frame_info.height * bytes_per_sample *
                                 frame_info.component_count);
    read(pnm_file, input_buffer);
    if (!pnm_file.good())
        return false;

    // PNM format is stored with most significant byte first (big endian).
    if (bytes_per_sample == 2)
    {
        for (auto i{input_buffer.begin()}; i != input_buffer.end(); i += 2)
        {
            iter_swap(i, i + 1);
        }
    }

    jpegls_encoder encoder;
    encoder.frame_info(frame_info)
        .interleave_mode(frame_info.component_count == 3 ? interleave_mode::line : interleave_mode::none);

    vector<uint8_t> destination(encoder.estimated_destination_size());
    encoder.destination(destination);
    const size_t bytes_encoded{encoder.encode(input_buffer)};

    ofstream jls_file_stream(open_output_stream(filename_output));
    write(jls_file_stream, destination, bytes_encoded);
    jls_file_stream.close();

    return true;
}
catch (const runtime_error& error)
{
    cout << "Failed to encode " << filename_input << " to " << filename_output << ", reason: " << error.what() << '\n';
    return false;
}


bool compare_pnm(istream& pnm_file1, istream& pnm_file2)
{
    const vector<int> header1{read_pnm_header(pnm_file1)};
    if (header1.size() != 4)
    {
        cout << "Cannot read header from input file 1\n";
        return false;
    }

    const vector<int> header2 = read_pnm_header(pnm_file2);
    if (header2.size() != 4)
    {
        cout << "Cannot read header from input file 2\n";
        return false;
    }

    if (header1[0] != header2[0])
    {
        cout << "Header type " << header1[0] << " is not equal with type " << header2[0] << "\n";
        return false;
    }

    const size_t width = header1[1];
    if (width != static_cast<size_t>(header2[1]))
    {
        cout << "Width " << width << " is not equal with width " << header2[1] << "\n";
        return false;
    }

    const size_t height = header1[2];
    if (height != static_cast<size_t>(header2[2]))
    {
        cout << "Height " << height << " is not equal with height " << header2[2] << "\n";
        return false;
    }

    if (header1[3] != header2[3])
    {
        cout << "max-value " << header1[3] << " is not equal with max-value " << header2[3] << "\n";
        return false;
    }
    const auto bytes_per_sample{header1[3] > 255 ? 2 : 1};

    const size_t byte_count{width * height * bytes_per_sample};
    vector<uint8_t> bytes1(byte_count);
    vector<uint8_t> bytes2(byte_count);

    read(pnm_file1, bytes1);
    read(pnm_file2, bytes2);

    for (size_t x{}; x != height; ++x)
    {
        for (size_t y{}; y < width; y += bytes_per_sample)
        {
            if (bytes_per_sample == 1)
            {
                if (bytes1[(x * width) + y] != bytes2[(x * width) + y])
                {
                    cout << "Values of the 2 files are different, height:" << x << ", width:" << y << "\n";
                    return false;
                }
            }
            else
            {
                if (bytes1[(x * width) + y] != bytes2[(x * width) + y] ||
                    bytes1[(x * width) + (y + 1)] != bytes2[(x * width) + (y + 1)])
                {
                    cout << "Values of the 2 files are different, height:" << x << ", width:" << y << "\n";
                    return false;
                }
            }
        }
    }

    cout << "Values of the 2 files are equal\n";
    return true;
}


bool decode_raw(const char* filename_encoded, const char* filename_output)
try
{
    const vector<uint8_t> encoded_source{read_file(filename_encoded)};
    vector<uint8_t> decoded_destination;
    jpegls_decoder::decode(encoded_source, decoded_destination);
    write_file(filename_output, decoded_destination.data(), decoded_destination.size());
    return true;
}
catch (const runtime_error& error)
{
    cout << "Failed to decode " << filename_encoded << " to " << filename_output << ", reason: " << error.what() << '\n';
    return false;
}


void test_encode_from_stream()
{
    test_encode_from_stream("test/0015.raw", 0, 1024, 1024, 8, 1, interleave_mode::none, 0x3D3ee);
    test_encode_from_stream("test/conformance/test8.ppm", 15, 256, 256, 8, 3, interleave_mode::sample, 99734);
    test_encode_from_stream("test/conformance/test8.ppm", 15, 256, 256, 8, 3, interleave_mode::line, 100615);
}


bool unit_test()
{
    try
    {
        cout << "Test Conformance\n";
        test_encode_from_stream();
        test_conformance();

        test_decode_rect();

        cout << "Test Traits\n";
        test_traits16_bit();
        test_traits8_bit();

        cout << "Windows bitmap BGR/BGRA output\n";
        test_bgr();
        test_bgra();

        cout << "Test Small buffer\n";
        test_too_small_output_buffer();

        test_fail_on_too_small_output_buffer();

        cout << "Test Color transform equivalence on HP images\n";
        test_color_transforms_hp_images();

        cout << "Test Annex H3\n";
        test_sample_annex_h3();

        test_noise_image();
        test_noise_image_with_custom_reset();

        cout << "Test robustness\n";
        test_decode_bit_stream_with_no_marker_start();
        test_decode_bit_stream_with_unsupported_encoding();
        test_decode_bit_stream_with_unknown_jpeg_marker();

        cout << "Test Legacy API\n";
        test_legacy_api();

        return true;
    }
    catch (const unit_test_exception&)
    {
        cout << "==> Unit test failed <==\n";
    }
    catch (const std::runtime_error& error)
    {
        cout << "==> Unit test failed due to external problem: " << error.what() << "\n";
    }

    return false;
}

} // namespace


int main(const int argc, const char* const argv[]) // NOLINT(bugprone-exception-escape)
{
    if (argc == 1)
    {
        cout << "CharLS test runner.\nOptions: -unittest, -bitstreamdamage, -performance[:loop-count], "
                "-decodeperformance[:loop-count], -decoderaw -encodepnm -decodetopnm -comparepnm -legacy\n";
        return EXIT_FAILURE;
    }

    for (int i{1}; i != argc; ++i)
    {
        const string str{argv[i]};
        if (str == "-unittest")
        {
            return result_to_exit_code(unit_test());
        }

        if (str == "-decoderaw")
        {
            if (i != 1 || argc != 4)
            {
                cout << "Syntax: -decoderaw input-file output-file\n";
                return EXIT_FAILURE;
            }
            return result_to_exit_code(decode_raw(argv[2], argv[3]));
        }

        if (str == "-decodetopnm")
        {
            if (i != 1 || argc != 4)
            {
                cout << "Syntax: -decodetopnm input-file output-file\n";
                return EXIT_FAILURE;
            }

            return result_to_exit_code(decode_to_pnm(argv[2], argv[3]));
        }

        if (str == "-encodepnm")
        {
            if (i != 1 || argc != 4)
            {
                cout << "Syntax: -encodepnm input-file output-file\n";
                return EXIT_FAILURE;
            }

            return result_to_exit_code(encode_pnm(argv[2], argv[3]));
        }

        if (str == "-comparepnm")
        {
            if (i != 1 || argc != 4)
            {
                cout << "Syntax: -encodepnm input-file output-file\n";
                return EXIT_FAILURE;
            }
            ifstream pnm_file1(argv[2], mode_input);
            ifstream pnm_file2(argv[3], mode_input);

            return result_to_exit_code(compare_pnm(pnm_file1, pnm_file2));
        }

        if (str == "-bitstreamdamage")
        {
            damaged_bit_stream_tests();
            continue;
        }

        if (str.compare(0, 12, "-performance") == 0)
        {
            int loop_count{1};

            // Extract the optional loop count from the command line. Longer running tests make the measurements more
            // reliable.
            auto index = str.find(':');
            if (index != string::npos)
            {
                loop_count = stoi(str.substr(++index));
                if (loop_count < 1)
                {
                    cout << "Loop count not understood or invalid: %s" << str << "\n";
                    break;
                }
            }

            performance_tests(loop_count);
            continue;
        }

        if (str.compare(0, 17, "-rgb8_performance") == 0)
        {
            // See the comments in function, how to prepare this test.
            test_large_image_performance_rgb8(1);
            continue;
        }

        if (str.compare(0, 18, "-decodeperformance") == 0)
        {
            int loop_count{1};

            // Extract the optional loop count from the command line. Longer running tests make the measurements more
            // reliable.
            auto index = str.find(':');
            if (index != string::npos)
            {
                loop_count = stoi(str.substr(++index));
                if (loop_count < 1)
                {
                    cout << "Loop count not understood or invalid: " << str << "\n";
                    break;
                }
            }

            decode_performance_tests(loop_count);
            continue;
        }

        if (str.compare(0, 19, "-encode-performance") == 0)
        {
            int loop_count{1};

            // Extract the optional loop count from the command line. Longer running tests make the measurements more
            // reliable.
            auto index = str.find(':');
            if (index != string::npos)
            {
                loop_count = stoi(str.substr(++index));
                if (loop_count < 1)
                {
                    cout << "Loop count not understood or invalid: " << str << "\n";
                    break;
                }
            }

            encode_performance_tests(loop_count);
            continue;
        }

        if (str == "-dicom")
        {
            test_dicom_wg4_images();
            continue;
        }

        if (str == "-legacy")
        {
            test_legacy_api();
            continue;
        }

        cout << "Option not understood: " << argv[i] << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
