// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "util.hpp"

#include <charls/charls.hpp>

#include <vector>

#include "../test/portable_anymap_file.hpp"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::vector;
using namespace charls_test;

namespace charls::test {

namespace {

// The following functions are used as sample code in the documentation
// Ensure that the code compiles and works by unit testing it.

std::vector<std::byte> decode_simple_8_bit_monochrome(const std::vector<std::byte>& source)
{
    std::vector<std::byte> destination;

    if (const auto [frame_info, _] = jpegls_decoder::decode(source, destination);
        frame_info.component_count != 1 || frame_info.bits_per_sample != 8)
        throw std::runtime_error("Not a 8 bit monochrome image");

    return destination;
}

std::vector<std::byte> decode_advanced(const std::vector<std::byte>& source)
{
    jpegls_decoder decoder{source, true};

    // Standalone JPEG-LS files may have a SPIFF header (color space info, etc.)
    if (decoder.spiff_header_has_value() && decoder.spiff_header().color_space != spiff_color_space::grayscale)
        throw std::runtime_error("Not a grayscale image");

    // After read_header() other properties can also be retrieved.
    if (decoder.get_near_lossless() != 0)
    {
        // Handle lossy images.
    }

    return decoder.decode<std::vector<std::byte>>();
}

std::vector<std::byte> encode_simple_8_bit_monochrome(const std::vector<std::byte>& source, const uint32_t width,
                                                      const uint32_t height)
{
    constexpr auto bits_per_sample{8};
    constexpr auto component_count{1};
    return jpegls_encoder::encode(source, {width, height, bits_per_sample, component_count});
}

std::vector<std::byte> encode_advanced_8_bit_monochrome(const std::vector<std::byte>& source, const uint32_t width,
                                                        const uint32_t height)
{
    jpegls_encoder encoder;
    encoder.frame_info({width, height, 8, 1})
           .encoding_options(encoding_options::include_version_number);

    std::vector<std::byte> destination(encoder.estimated_destination_size());
    encoder.destination(destination);

    encoder.write_standard_spiff_header(spiff_color_space::grayscale);

    const size_t bytes_written{encoder.encode(source)};
    destination.resize(bytes_written);

    return destination;
}

} // namespace


TEST_CLASS(documentation_test)
{
public:
    TEST_METHOD(call_decode_simple_8_bit_monochrome) // NOLINT
    {
        const auto source{read_file("DataFiles/tulips-gray-8bit-512-512-hp-encoder.jls")};
        const auto charls_decoded{decode_simple_8_bit_monochrome(source)};

        test_decoded_data(charls_decoded, "DataFiles/tulips-gray-8bit-512-512.pgm");
    }

    TEST_METHOD(call_decode_advanced) // NOLINT
    {
        const auto source{read_file("DataFiles/tulips-gray-8bit-512-512-hp-encoder.jls")};
        const auto charls_decoded{decode_advanced(source)};

        test_decoded_data(charls_decoded, "DataFiles/tulips-gray-8bit-512-512.pgm");
    }

    TEST_METHOD(call_encode_simple_8_bit_monochrome) // NOLINT
    {
        portable_anymap_file reference_file("DataFiles/tulips-gray-8bit-512-512.pgm");
        const auto charls_encoded{encode_simple_8_bit_monochrome(reference_file.image_data(),
                                                                 static_cast<uint32_t>(reference_file.width()),
                                                                 static_cast<uint32_t>(reference_file.height()))};

        test_by_decoding(charls_encoded, reference_file, interleave_mode::none);
    }

    TEST_METHOD(call_encode_advanced_8_bit_monochrome) // NOLINT
    {
        portable_anymap_file reference_file("DataFiles/tulips-gray-8bit-512-512.pgm");
        const auto charls_encoded{encode_advanced_8_bit_monochrome(reference_file.image_data(),
                                                                   static_cast<uint32_t>(reference_file.width()),
                                                                   static_cast<uint32_t>(reference_file.height()))};

        test_by_decoding(charls_encoded, reference_file, interleave_mode::none);
    }

private:
    static void test_decoded_data(const vector<std::byte>& decoded_source, const char* raw_filename)
    {
        portable_anymap_file reference_file{read_anymap_reference_file(raw_filename, interleave_mode::none)};
        const vector<std::byte>& uncompressed_source{reference_file.image_data()};

        Assert::AreEqual(decoded_source.size(), uncompressed_source.size());

        for (size_t i{}; i != uncompressed_source.size(); ++i)
        {
            if (decoded_source[i] != uncompressed_source[i]) // AreEqual is very slow, pre-test to speed up 50X
            {
                Assert::AreEqual(decoded_source[i], uncompressed_source[i]);
            }
        }
    }

    static void test_by_decoding(const vector<std::byte>& encoded_source, const portable_anymap_file& reference_file,
                                 const interleave_mode interleave_mode)
    {
        jpegls_decoder decoder;
        decoder.source(encoded_source);
        decoder.read_header();

        const auto& frame_info{decoder.frame_info()};
        Assert::AreEqual(static_cast<uint32_t>(reference_file.width()), frame_info.width);
        Assert::AreEqual(static_cast<uint32_t>(reference_file.height()), frame_info.height);
        Assert::AreEqual(reference_file.component_count(), frame_info.component_count);
        Assert::AreEqual(reference_file.bits_per_sample(), frame_info.bits_per_sample);
        Assert::AreEqual(interleave_mode, decoder.get_interleave_mode());

        vector<std::byte> destination(decoder.get_destination_size());
        decoder.decode(destination);

        const vector<std::byte>& uncompressed_source{reference_file.image_data()};

        Assert::AreEqual(destination.size(), uncompressed_source.size());

        for (size_t i{}; i != uncompressed_source.size(); ++i)
        {
            if (destination[i] != uncompressed_source[i]) // AreEqual is very slow, pre-test to speed up 50X
            {
                Assert::AreEqual(destination[i], uncompressed_source[i]);
            }
        }
    }
};

} // namespace charls::test
