// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "util.h"

#include <charls/charls.h>

#include <array>
#include <tuple>
#include <vector>

#include "../test/portable_anymap_file.h"

// ReSharper disable CppDeprecatedEntity
DISABLE_DEPRECATED_WARNING

namespace {

// The following functions are used as sample code in the documentation
// Ensure that the code compiles and works by unit testing it.

std::vector<uint8_t> decode_simple_8_bit_monochrome(const std::vector<uint8_t>& source)
{
    std::vector<uint8_t> destination;

    charls::frame_info frame_info;
    std::tie(frame_info, std::ignore) = charls::jpegls_decoder::decode(source, destination);

    if (frame_info.component_count != 1 || frame_info.bits_per_sample != 8)
        throw std::exception("Not a 8 bit monochrome image");

    return destination;
}

std::vector<uint8_t> decode_advanced(const std::vector<uint8_t>& source)
{
    const charls::jpegls_decoder decoder{source, true};

    // Standalone JPEG-LS files may have a SPIFF header (color space info, etc.)
    if (decoder.spiff_header_has_value() && decoder.spiff_header().color_space != charls::spiff_color_space::grayscale)
        throw std::exception("Not a grayscale image");

    // After read_header() other properties can also be retrieved.
    if (decoder.near_lossless() != 0)
    {
        // Handle lossy images.
    }

    return decoder.decode<std::vector<uint8_t>>();
}

std::vector<uint8_t> decode_simple_8_bit_monochrome_legacy(const std::vector<uint8_t>& source)
{
    std::array<char, ErrorMessageSize> error_message{};
    JlsParameters parameters{};
    auto error = JpegLsReadHeader(source.data(), source.size(), &parameters, error_message.data());
    if (error != CharlsApiResultType::OK)
        throw std::exception(error_message.data());

    if (parameters.components != 1 || parameters.bitsPerSample != 8)
        throw std::exception("Not a 8 bit monochrome image");

    const size_t destination_size = static_cast<size_t>(parameters.width) * static_cast<uint32_t>(parameters.height);
    std::vector<uint8_t> destination(destination_size);

    error = JpegLsDecode(destination.data(), destination.size(), source.data(), source.size(), &parameters,
                         error_message.data());
    if (error != CharlsApiResultType::OK)
        throw std::exception(error_message.data());

    return destination;
}

std::vector<uint8_t> encode_simple_8_bit_monochrome(const std::vector<uint8_t>& source, const uint32_t width,
                                                    const uint32_t height)
{
    constexpr auto bits_per_sample = 8;
    constexpr auto component_count = 1;
    return charls::jpegls_encoder::encode(source, {width, height, bits_per_sample, component_count});
}

std::vector<uint8_t> encode_advanced_8_bit_monochrome(const std::vector<uint8_t>& source, const uint32_t width,
                                                      const uint32_t height)
{
    charls::jpegls_encoder encoder;
    encoder.frame_info({width, height, 8, 1});

    std::vector<uint8_t> destination(encoder.estimated_destination_size());
    encoder.destination(destination);

    encoder.write_standard_spiff_header(charls::spiff_color_space::grayscale);

    const size_t bytes_written{encoder.encode(source)};
    destination.resize(bytes_written);

    return destination;
}

std::vector<uint8_t> encode_simple_8_bit_monochrome_legacy(const std::vector<uint8_t>& source, const uint32_t width,
                                                           const uint32_t height)
{
    std::array<char, ErrorMessageSize> error_message{};
    JlsParameters parameters{};
    parameters.width = static_cast<int32_t>(width);
    parameters.height = static_cast<int32_t>(height);
    parameters.bitsPerSample = 8;
    parameters.components = 1;

    const size_t estimated_destination_size = static_cast<size_t>(width) * height * 1 * 1 + 1024;
    std::vector<uint8_t> destination(estimated_destination_size);

    size_t bytes_written;
    const auto error = JpegLsEncode(destination.data(), destination.size(), &bytes_written, source.data(), source.size(),
                                    &parameters, error_message.data());
    if (error != CharlsApiResultType::OK)
        throw std::exception(error_message.data());

    destination.resize(bytes_written);
    return destination;
}

} // namespace

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::vector;
using namespace charls_test;

namespace charls { namespace test {

TEST_CLASS(documentation_test)
{
public:
    TEST_METHOD(call_decode_simple_8_bit_monochrome) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/lena8b.jls")};

        const vector<uint8_t> charls_decoded = decode_simple_8_bit_monochrome(source);

        test_decoded_data(charls_decoded, "DataFiles/lena8b.pgm");
    }

    TEST_METHOD(call_decode_advanced) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/lena8b.jls")};

        const vector<uint8_t> charls_decoded = decode_advanced(source);

        test_decoded_data(charls_decoded, "DataFiles/lena8b.pgm");
    }

    TEST_METHOD(call_decode_simple_8_bit_monochrome_legacy) // NOLINT
    {
        const vector<uint8_t> source{read_file("DataFiles/lena8b.jls")};

        const vector<uint8_t> charls_decoded = decode_simple_8_bit_monochrome_legacy(source);

        test_decoded_data(charls_decoded, "DataFiles/lena8b.pgm");
    }

    TEST_METHOD(call_encode_simple_8_bit_monochrome) // NOLINT
    {
        portable_anymap_file reference_file("DataFiles/lena8b.pgm");

        const vector<uint8_t> charls_encoded =
            encode_simple_8_bit_monochrome(reference_file.image_data(), static_cast<uint32_t>(reference_file.width()),
                                           static_cast<uint32_t>(reference_file.height()));

        test_by_decoding(charls_encoded, reference_file, interleave_mode::none);
    }

    TEST_METHOD(call_encode_advanced_8_bit_monochrome) // NOLINT
    {
        portable_anymap_file reference_file("DataFiles/lena8b.pgm");

        const vector<uint8_t> charls_encoded =
            encode_advanced_8_bit_monochrome(reference_file.image_data(), static_cast<uint32_t>(reference_file.width()),
                                             static_cast<uint32_t>(reference_file.height()));

        test_by_decoding(charls_encoded, reference_file, interleave_mode::none);
    }

    TEST_METHOD(call_encode_simple_8_bit_monochrome_legacy) // NOLINT
    {
        portable_anymap_file reference_file("DataFiles/lena8b.pgm");

        const vector<uint8_t> charls_encoded =
            encode_simple_8_bit_monochrome_legacy(reference_file.image_data(), static_cast<uint32_t>(reference_file.width()),
                                                  static_cast<uint32_t>(reference_file.height()));

        test_by_decoding(charls_encoded, reference_file, interleave_mode::none);
    }

private:
    static void test_decoded_data(const vector<uint8_t>& decoded_source, const char* raw_filename)
    {
        portable_anymap_file reference_file = read_anymap_reference_file(raw_filename, interleave_mode::none);
        const vector<uint8_t>& uncompressed_source = reference_file.image_data();

        Assert::AreEqual(decoded_source.size(), uncompressed_source.size());

        for (size_t i = 0; i < uncompressed_source.size(); ++i)
        {
            if (decoded_source[i] != uncompressed_source[i]) // AreEqual is very slow, pre-test to speed up 50X
            {
                Assert::AreEqual(decoded_source[i], uncompressed_source[i]);
            }
        }
    }

    static void test_by_decoding(const vector<uint8_t>& encoded_source, const portable_anymap_file& reference_file,
                                 const interleave_mode interleave_mode)
    {
        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        const auto& frame_info = decoder.frame_info();
        Assert::AreEqual(static_cast<uint32_t>(reference_file.width()), frame_info.width);
        Assert::AreEqual(static_cast<uint32_t>(reference_file.height()), frame_info.height);
        Assert::AreEqual(reference_file.component_count(), frame_info.component_count);
        Assert::AreEqual(reference_file.bits_per_sample(), frame_info.bits_per_sample);
        Assert::AreEqual(interleave_mode, decoder.interleave_mode());

        vector<uint8_t> destination(decoder.destination_size());
        decoder.decode(destination);

        const vector<uint8_t>& uncompressed_source = reference_file.image_data();

        Assert::AreEqual(destination.size(), uncompressed_source.size());

        for (size_t i = 0; i < uncompressed_source.size(); ++i)
        {
            if (destination[i] != uncompressed_source[i]) // AreEqual is very slow, pre-test to speed up 50X
            {
                Assert::AreEqual(destination[i], uncompressed_source[i]);
            }
        }
    }
};

}} // namespace charls::test

// ReSharper restore CppDeprecatedEntity
RESTORE_DEPRECATED_WARNING
