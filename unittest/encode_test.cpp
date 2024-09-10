// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "util.hpp"

#include <charls/charls.hpp>

#include "../test/portable_anymap_file.hpp"

#include <array>

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using namespace charls_test;
using std::array;
using std::byte;
using std::vector;

namespace charls::test {

TEST_CLASS(encode_test)
{
public:
    TEST_METHOD(encode_monochrome_2_bit_lossless) // NOLINT
    {
        encode("DataFiles/2bit_parrot_150x200.pgm", 2866);
    }

    TEST_METHOD(encode_monochrome_4_bit_lossless) // NOLINT
    {
        encode("DataFiles/4bit-monochrome.pgm", 1596);
    }

    TEST_METHOD(encode_monochrome_12_bit_lossless) // NOLINT
    {
        encode("DataFiles/test16.pgm", 60077);
    }

    TEST_METHOD(encode_monochrome_16_bit_lossless) // NOLINT
    {
        encode("DataFiles/16-bit-640-480-many-dots.pgm", 4138);
    }

    TEST_METHOD(encode_2_components_8_bit_interleave_none)
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}, byte{70}, byte{80}};
        encode({2, 2, 8, 2}, {data.cbegin(), data.cend()}, 53, interleave_mode::none);
    }

    TEST_METHOD(encode_2_components_8_bit_interleave_line)
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}, byte{70}, byte{80}};
        encode({2, 2, 8, 2}, {data.cbegin(), data.cend()}, 43, interleave_mode::line);
    }

    TEST_METHOD(encode_2_components_8_bit_interleave_sample)
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}, byte{70}, byte{80}};
        encode({2, 2, 8, 2}, {data.cbegin(), data.cend()}, 43, interleave_mode::sample);
    }

    TEST_METHOD(encode_2_components_16_bit_interleave_none)
    {
        constexpr array data{byte{10}, byte{1}, byte{20}, byte{1}, byte{30}, byte{1}, byte{40}, byte{1},
                             byte{50}, byte{1}, byte{60}, byte{1}, byte{70}, byte{1}, byte{80}, byte{1}};
        encode({2, 2, 16, 2}, {data.cbegin(), data.cend()}, 52, interleave_mode::none);
    }

    TEST_METHOD(encode_2_components_16_bit_interleave_line)
    {
        constexpr array data{byte{10}, byte{1}, byte{20}, byte{1}, byte{30}, byte{1}, byte{40}, byte{1},
                             byte{50}, byte{1}, byte{60}, byte{1}, byte{70}, byte{1}, byte{80}, byte{1}};
        encode({2, 2, 16, 2}, {data.cbegin(), data.cend()}, 44, interleave_mode::line);
    }

    TEST_METHOD(encode_2_components_16_bit_interleave_sample)
    {
        constexpr array data{byte{10}, byte{1}, byte{20}, byte{1}, byte{30}, byte{1}, byte{40}, byte{1},
                             byte{50}, byte{1}, byte{60}, byte{1}, byte{70}, byte{1}, byte{80}, byte{1}};
        encode({2, 2, 16, 2}, {data.cbegin(), data.cend()}, 44, interleave_mode::sample);
    }

    TEST_METHOD(encode_color_8_bit_interleave_none_lossless) // NOLINT
    {
        encode("DataFiles/test8.ppm", 102248);
    }

    TEST_METHOD(encode_color_8_bit_interleave_line_lossless) // NOLINT
    {
        encode("DataFiles/test8.ppm", 100615, interleave_mode::line);
    }

    TEST_METHOD(encode_color_8_bit_interleave_sample_lossless) // NOLINT
    {
        encode("DataFiles/test8.ppm", 99734, interleave_mode::sample);
    }

    TEST_METHOD(encode_color_8_bit_interleave_line_hp1) // NOLINT
    {
        encode("DataFiles/test8.ppm", 91617, interleave_mode::line, color_transformation::hp1);
    }

    TEST_METHOD(encode_color_8_bit_interleave_sample_hp1) // NOLINT
    {
        encode("DataFiles/test8.ppm", 91463, interleave_mode::sample, color_transformation::hp1);
    }

    TEST_METHOD(encode_color_8_bit_interleave_line_hp2) // NOLINT
    {
        encode("DataFiles/test8.ppm", 91693, interleave_mode::line, color_transformation::hp2);
    }

    TEST_METHOD(encode_color_8_bit_interleave_sample_hp2) // NOLINT
    {
        encode("DataFiles/test8.ppm", 91457, interleave_mode::sample, color_transformation::hp2);
    }

    TEST_METHOD(encode_color_8_bit_interleave_line_hp3) // NOLINT
    {
        encode("DataFiles/test8.ppm", 91993, interleave_mode::line, color_transformation::hp3);
    }

    TEST_METHOD(encode_color_8_bit_interleave_sample_hp3) // NOLINT
    {
        encode("DataFiles/test8.ppm", 91862, interleave_mode::sample, color_transformation::hp3);
    }

    TEST_METHOD(encode_monchrome_16_bit_interleave_none) // NOLINT
    {
        constexpr array data{byte{}, byte{10}, byte{}, byte{20}, byte{}, byte{30}, byte{}, byte{40}};
        encode({2, 2, 16, 1}, {data.cbegin(), data.cend()}, 36, interleave_mode::none);
    }

    TEST_METHOD(encode_color_16_bit_interleave_none) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}};
        encode({1, 1, 16, 3}, {data.cbegin(), data.cend()}, 66, interleave_mode::none);
    }

    TEST_METHOD(encode_color_16_bit_interleave_line) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}};
        encode({1, 1, 16, 3}, {data.cbegin(), data.cend()}, 45, interleave_mode::line);
    }

    TEST_METHOD(encode_color_16_bit_interleave_sample) // NOLINT
    {
        constexpr array data{byte{},  byte{},   byte{},  byte{},   byte{},  byte{},    // row 0, pixel 0
                             byte{},  byte{},   byte{},  byte{},   byte{},  byte{},    // row 0, pixel 1
                             byte{1}, byte{10}, byte{1}, byte{20}, byte{1}, byte{30},  // row 1, pixel 0
                             byte{1}, byte{40}, byte{1}, byte{50}, byte{1}, byte{60}}; // row 1, pixel 1
        encode({2, 2, 16, 3}, {data.cbegin(), data.cend()}, 51, interleave_mode::sample);
    }

    TEST_METHOD(encode_color_16_bit_interleave_line_hp1) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}};
        encode({1, 1, 16, 3}, {data.cbegin(), data.cend()}, 59, interleave_mode::line, color_transformation::hp1);
    }

    TEST_METHOD(encode_color_16_bit_interleave_sample_hp1) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}};
        encode({1, 1, 16, 3}, {data.cbegin(), data.cend()}, 59, interleave_mode::sample, color_transformation::hp1);
    }

    TEST_METHOD(encode_color_16_bit_interleave_line_hp2) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}};
        encode({1, 1, 16, 3}, {data.cbegin(), data.cend()}, 59, interleave_mode::line, color_transformation::hp2);
    }

    TEST_METHOD(encode_color_16_bit_interleave_sample_hp2) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}};
        encode({1, 1, 16, 3}, {data.cbegin(), data.cend()}, 59, interleave_mode::sample, color_transformation::hp2);
    }

    TEST_METHOD(encode_color_16_bit_interleave_line_hp3) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}};
        encode({1, 1, 16, 3}, {data.cbegin(), data.cend()}, 55, interleave_mode::line, color_transformation::hp3);
    }

    TEST_METHOD(encode_color_16_bit_interleave_sample_hp3) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}};
        encode({1, 1, 16, 3}, {data.cbegin(), data.cend()}, 55, interleave_mode::sample, color_transformation::hp3);
    }

    TEST_METHOD(encode_4_components_8_bit_interleave_none) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}};
        encode({1, 1, 8, 4}, {data.cbegin(), data.cend()}, 75, interleave_mode::none);
    }

    TEST_METHOD(encode_4_components_8_bit_interleave_line) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}};
        encode({1, 1, 8, 4}, {data.cbegin(), data.cend()}, 47, interleave_mode::line);
    }

    TEST_METHOD(encode_4_components_8_bit_interleave_sample) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}};
        encode({1, 1, 8, 4}, {data.cbegin(), data.cend()}, 47, interleave_mode::sample);
    }

    TEST_METHOD(encode_4_components_16_bit_interleave_none) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}, byte{70}, byte{80}};
        encode({1, 1, 16, 4}, {data.cbegin(), data.cend()}, 86, interleave_mode::none);
    }

    TEST_METHOD(encode_4_components_16_bit_interleave_line) // NOLINT
    {
        constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}, byte{70}, byte{80}};
        encode({1, 1, 16, 4}, {data.cbegin(), data.cend()}, 52, interleave_mode::line);
    }

    TEST_METHOD(encode_4_components_16_bit_interleave_sample) // NOLINT
    {
        constexpr array data{byte{},  byte{},   byte{},  byte{},   byte{},  byte{},   byte{},  byte{},    // row 0, pixel 0
                             byte{},  byte{},   byte{},  byte{},   byte{},  byte{},   byte{},  byte{},    // row 0, pixel 1
                             byte{1}, byte{10}, byte{1}, byte{20}, byte{1}, byte{30}, byte{1}, byte{40},  // row 1, pixel 0
                             byte{1}, byte{50}, byte{1}, byte{60}, byte{1}, byte{70}, byte{1}, byte{80}}; // row 1, pixel 1


        // constexpr array data{byte{10}, byte{20}, byte{30}, byte{40}, byte{50}, byte{60}, byte{70}, byte{80}};
        encode({2, 2, 16, 4}, {data.cbegin(), data.cend()}, 61, interleave_mode::sample);
    }

    TEST_METHOD(encode_with_different_lossless_values) // NOLINT
    {
        jpegls_encoder encoder;
        encoder.frame_info({2, 2, 8, 3});

        constexpr array data{byte{24}, byte{23}, byte{22}, byte{21}};

        vector<byte> encoded_data(encoder.estimated_destination_size());
        encoder.destination(encoded_data);

        encoder.near_lossless(0);
        encoder.encode_components(data, 1);
        encoder.near_lossless(2);
        encoder.encode_components(data, 1);
        encoder.near_lossless(10);
        encoder.encode_components(data, 1);

        jpegls_decoder decoder(encoded_data, true);

        vector<byte> destination(decoder.get_destination_size());
        decoder.decode(destination);

        check_output(data.data(), data.size(), destination.data(), decoder, 3,
                     static_cast<size_t>(decoder.frame_info().height) * decoder.frame_info().width);

        Assert::AreEqual(0, decoder.get_near_lossless(0));
        Assert::AreEqual(2, decoder.get_near_lossless(1));
        Assert::AreEqual(10, decoder.get_near_lossless(2));
    }

    TEST_METHOD(encode_with_different_preset_coding_parameters) // NOLINT
    {
        jpegls_encoder encoder;
        encoder.frame_info({2, 2, 8, 3});

        constexpr array data{byte{24}, byte{23}, byte{22}, byte{21}};

        vector<byte> encoded_data(encoder.estimated_destination_size());
        encoder.destination(encoded_data);

        encoder.preset_coding_parameters({});
        encoder.encode_components(data, 1);
        encoder.preset_coding_parameters({25, 10, 20, 22, 64});
        encoder.encode_components(data, 1);
        encoder.preset_coding_parameters({25, 0, 0, 0, 3});
        encoder.encode_components(data, 1);

        jpegls_decoder decoder(encoded_data, true);

        vector<byte> destination(decoder.get_destination_size());
        decoder.decode(destination);

        check_output(data.data(), data.size(), destination.data(), decoder, 3,
                     static_cast<size_t>(decoder.frame_info().height) * decoder.frame_info().width);
    }

    TEST_METHOD(encode_with_different_interleave_modes_none_first) // NOLINT
    {
        jpegls_encoder encoder;
        encoder.frame_info({8, 2, 8, 4});

        constexpr array component0{byte{24}, byte{23}, byte{22}, byte{21}, byte{20}, byte{19}, byte{18}, byte{17},
                                   byte{16}, byte{15}, byte{14}, byte{13}, byte{12}, byte{11}, byte{10}, byte{9}};

        constexpr array component_1_and_2_and_3{
            byte{24}, byte{16}, byte{23}, byte{15}, byte{22}, byte{14}, byte{21}, byte{13}, byte{20}, byte{12},
            byte{19}, byte{11}, byte{18}, byte{10}, byte{17}, byte{9},  byte{24}, byte{16}, byte{23}, byte{15},
            byte{22}, byte{14}, byte{21}, byte{13}, byte{20}, byte{12}, byte{19}, byte{11}, byte{18}, byte{10},
            byte{17}, byte{9},  byte{24}, byte{16}, byte{23}, byte{15}, byte{22}, byte{14}, byte{21}, byte{13},
            byte{20}, byte{12}, byte{19}, byte{11}, byte{18}, byte{10}, byte{17}, byte{9}};

        vector<byte> encoded_data(encoder.estimated_destination_size());
        encoder.destination(encoded_data);

        encoder.interleave_mode(interleave_mode::none);
        encoder.encode_components(component0, 1);
        encoder.interleave_mode(interleave_mode::sample);
        encoder.encode_components(component_1_and_2_and_3, 3);

        jpegls_decoder decoder(encoded_data, true);

        vector<byte> destination(decoder.get_destination_size());
        decoder.decode(destination);

        check_output(component0.data(), component0.size(), destination.data(), decoder, 1, 8 * 2);
        check_output(component_1_and_2_and_3.data(), component_1_and_2_and_3.size(), destination.data() + 8 * 2, decoder, 1,
                     8 * 2 * 3);
        Assert::AreEqual(interleave_mode::none, decoder.get_interleave_mode(0));
        Assert::AreEqual(interleave_mode::sample, decoder.get_interleave_mode(1));
        Assert::AreEqual(interleave_mode::sample, decoder.get_interleave_mode(2));
        Assert::AreEqual(interleave_mode::sample, decoder.get_interleave_mode(3));
    }

    TEST_METHOD(encode_with_different_interleave_modes_sample_first) // NOLINT
    {
        jpegls_encoder encoder;
        encoder.frame_info({8, 2, 8, 4});

        constexpr array component_0_and_1_and_2{
            byte{24}, byte{16}, byte{23}, byte{15}, byte{22}, byte{14}, byte{21}, byte{13}, byte{20}, byte{12},
            byte{19}, byte{11}, byte{18}, byte{10}, byte{17}, byte{9},  byte{24}, byte{16}, byte{23}, byte{15},
            byte{22}, byte{14}, byte{21}, byte{13}, byte{20}, byte{12}, byte{19}, byte{11}, byte{18}, byte{10},
            byte{17}, byte{9},  byte{24}, byte{16}, byte{23}, byte{15}, byte{22}, byte{14}, byte{21}, byte{13},
            byte{20}, byte{12}, byte{19}, byte{11}, byte{18}, byte{10}, byte{17}, byte{9}};

        constexpr array component3{byte{24}, byte{23}, byte{22}, byte{21}, byte{20}, byte{19}, byte{18}, byte{17},
                                   byte{16}, byte{15}, byte{14}, byte{13}, byte{12}, byte{11}, byte{10}, byte{9}};

        vector<byte> encoded_data(encoder.estimated_destination_size());
        encoder.destination(encoded_data);

        encoder.interleave_mode(interleave_mode::sample);
        encoder.encode_components(component_0_and_1_and_2, 3);
        encoder.interleave_mode(interleave_mode::none);
        encoder.encode_components(component3, 1);

        jpegls_decoder decoder(encoded_data, true);

        vector<byte> destination(decoder.get_destination_size());
        decoder.decode(destination);

        check_output(component_0_and_1_and_2.data(), component_0_and_1_and_2.size(), destination.data(), decoder, 1,
                     8 * 2 * 3);
        check_output(component3.data(), component3.size(), destination.data() + 8 * 2 * 3, decoder, 1, 8 * 2);
        Assert::AreEqual(interleave_mode::sample, decoder.get_interleave_mode(0));
        Assert::AreEqual(interleave_mode::sample, decoder.get_interleave_mode(1));
        Assert::AreEqual(interleave_mode::sample, decoder.get_interleave_mode(2));
        Assert::AreEqual(interleave_mode::none, decoder.get_interleave_mode(3));
    }

private:
    static void check_output(const byte* source, const size_t source_size, const byte* destination,
                             const jpegls_decoder& decoder, const int component_count, const size_t component_size)
    {
        for (int component = 0; component < component_count; ++component)
        {
            const byte* component_destination = destination + component_size * component;

            if (const int near_lossless = decoder.get_near_lossless(component); near_lossless == 0)
            {
                for (size_t i{}; i != source_size; ++i)
                {
                    Assert::AreEqual(source[i], component_destination[i]);
                }
            }
            else
            {
                for (size_t i{}; i != source_size; ++i)
                {
                    Assert::IsTrue(abs(static_cast<uint8_t>(source[i]) - static_cast<uint8_t>(component_destination[i])) <=
                                   near_lossless);
                }
            }
        }
    }

    static void encode(const char* filename, const size_t expected_size,
                       const interleave_mode interleave_mode = interleave_mode::none,
                       const color_transformation color_transformation = color_transformation::none)
    {
        const portable_anymap_file reference_file{read_anymap_reference_file(filename, interleave_mode)};

        encode({static_cast<uint32_t>(reference_file.width()), static_cast<uint32_t>(reference_file.height()),
                reference_file.bits_per_sample(), reference_file.component_count()},
               reference_file.image_data(), expected_size, interleave_mode, color_transformation);
    }

    static void encode(const frame_info& frame_info, const std::vector<byte>& source, const size_t expected_size,
                       const interleave_mode interleave_mode,
                       const color_transformation color_transformation = color_transformation::none)
    {
        jpegls_encoder encoder;
        encoder.frame_info(frame_info).interleave_mode(interleave_mode).color_transformation(color_transformation);

        vector<byte> encoded_data(encoder.estimated_destination_size());
        encoder.destination(encoded_data);

        const size_t bytes_written{encoder.encode(source)};
        Assert::AreEqual(expected_size, bytes_written);

        encoded_data.resize(bytes_written);
        test_by_decoding(encoded_data, frame_info, source, interleave_mode, color_transformation);
    }

    static void test_by_decoding(const vector<byte>& encoded_data, const frame_info& reference_frame_info,
                                 const std::vector<byte>& reference_source, const interleave_mode interleave_mode,
                                 const color_transformation color_transformation)
    {
        jpegls_decoder decoder;
        decoder.source(encoded_data);
        decoder.read_header();

        const auto& frame_info{decoder.frame_info()};
        Assert::AreEqual(reference_frame_info.width, frame_info.width);
        Assert::AreEqual(reference_frame_info.height, frame_info.height);
        Assert::AreEqual(reference_frame_info.bits_per_sample, frame_info.bits_per_sample);
        Assert::AreEqual(reference_frame_info.component_count, frame_info.component_count);
        Assert::IsTrue(interleave_mode == decoder.get_interleave_mode());
        Assert::IsTrue(color_transformation == decoder.color_transformation());

        vector<byte> destination(decoder.get_destination_size());
        decoder.decode(destination);

        Assert::AreEqual(destination.size(), reference_source.size());

        if (decoder.get_near_lossless() == 0)
        {
            for (size_t i{}; i != reference_source.size(); ++i)
            {
                if (destination[i] != reference_source[i]) // AreEqual is very slow, pre-test to speed up 50X
                {
                    Assert::AreEqual(destination[i], reference_source[i]);
                }
            }
        }
    }

    constexpr static size_t estimated_destination_size(const int width, const int height, const int component_count,
                                                       const int bits_per_sample) noexcept
    {
        return static_cast<size_t>(width) * height * component_count * (bits_per_sample < 9 ? 1 : 2) + 1024;
    }
};

} // namespace charls::test
