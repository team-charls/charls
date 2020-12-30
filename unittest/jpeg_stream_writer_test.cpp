// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "../src/jpeg_marker_code.h"
#include "../src/jpeg_stream_writer.h"

#include <array>

#include "util.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;
using std::array;

namespace charls { namespace test {

TEST_CLASS(jpeg_stream_writer_test)
{
public:
    TEST_METHOD(remaining_destination_will_be_zero_after_create_with_default) // NOLINT
    {
        const jpeg_stream_writer writer;
        Assert::AreEqual(static_cast<size_t>(0), writer.remaining_destination().size);
        Assert::IsNull(writer.remaining_destination().data);
    }

    TEST_METHOD(write_start_of_image) // NOLINT
    {
        array<uint8_t, 2> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        writer.write_start_of_image();

        Assert::AreEqual(static_cast<size_t>(2), writer.bytes_written());
        Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), buffer[1]);
    }

    TEST_METHOD(write_start_of_image_in_too_small_buffer) // NOLINT
    {
        array<uint8_t, 1> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::destination_buffer_too_small, [&] { writer.write_start_of_image(); });
        Assert::AreEqual(static_cast<size_t>(0), writer.bytes_written());
    }

    TEST_METHOD(write_end_of_image) // NOLINT
    {
        array<uint8_t, 2> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        writer.write_end_of_image();

        Assert::AreEqual(static_cast<size_t>(2), writer.bytes_written());
        Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::end_of_image), buffer[1]);
    }

    TEST_METHOD(write_end_of_image_in_too_small_buffer) // NOLINT
    {
        array<uint8_t, 1> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        assert_expect_exception(jpegls_errc::destination_buffer_too_small, [&] { writer.write_end_of_image(); });
        Assert::AreEqual(static_cast<size_t>(0), writer.bytes_written());
    }

    TEST_METHOD(write_spiff_segment) // NOLINT
    {
        array<uint8_t, 34> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        spiff_header header{spiff_profile_id::none,
                            3,
                            800,
                            600,
                            spiff_color_space::rgb,
                            8,
                            spiff_compression_type::jpeg_ls,
                            spiff_resolution_units::dots_per_inch,
                            96,
                            1024};

        writer.write_spiff_header_segment(header);

        Assert::AreEqual(static_cast<size_t>(34), writer.bytes_written());

        Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::application_data8), buffer[1]);

        Assert::AreEqual(static_cast<uint8_t>(0), buffer[2]);
        Assert::AreEqual(static_cast<uint8_t>(32), buffer[3]);

        // Verify SPIFF identifier string.
        Assert::AreEqual(static_cast<uint8_t>('S'), buffer[4]);
        Assert::AreEqual(static_cast<uint8_t>('P'), buffer[5]);
        Assert::AreEqual(static_cast<uint8_t>('I'), buffer[6]);
        Assert::AreEqual(static_cast<uint8_t>('F'), buffer[7]);
        Assert::AreEqual(static_cast<uint8_t>('F'), buffer[8]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[9]);

        // Verify version
        Assert::AreEqual(static_cast<uint8_t>(2), buffer[10]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[11]);

        Assert::AreEqual(static_cast<uint8_t>(header.profile_id), buffer[12]);
        Assert::AreEqual(static_cast<uint8_t>(header.component_count), buffer[13]);

        // Height
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[14]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[15]);
        Assert::AreEqual(static_cast<uint8_t>(0x3), buffer[16]);
        Assert::AreEqual(static_cast<uint8_t>(0x20), buffer[17]);

        // Width
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[18]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[19]);
        Assert::AreEqual(static_cast<uint8_t>(0x2), buffer[20]);
        Assert::AreEqual(static_cast<uint8_t>(0x58), buffer[21]);

        Assert::AreEqual(static_cast<uint8_t>(header.color_space), buffer[22]);
        Assert::AreEqual(static_cast<uint8_t>(header.bits_per_sample), buffer[23]);
        Assert::AreEqual(static_cast<uint8_t>(header.compression_type), buffer[24]);
        Assert::AreEqual(static_cast<uint8_t>(header.resolution_units), buffer[25]);

        // vertical_resolution
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[26]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[27]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[28]);
        Assert::AreEqual(static_cast<uint8_t>(96), buffer[29]);

        // header.horizontal_resolution = 1024;
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[30]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[31]);
        Assert::AreEqual(static_cast<uint8_t>(4), buffer[32]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[33]);
    }

    TEST_METHOD(write_spiff_segment_in_too_small_buffer) // NOLINT
    {
        array<uint8_t, 33> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        spiff_header header{spiff_profile_id::none,
                            3,
                            800,
                            600,
                            spiff_color_space::rgb,
                            8,
                            spiff_compression_type::jpeg_ls,
                            spiff_resolution_units::dots_per_inch,
                            96,
                            1024};

        assert_expect_exception(jpegls_errc::destination_buffer_too_small,
                                [&] { writer.write_spiff_header_segment(header); });
        Assert::AreEqual(static_cast<size_t>(0), writer.bytes_written());
    }

    TEST_METHOD(write_spiff_end_of_directory_segment) // NOLINT
    {
        array<uint8_t, 10> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        writer.write_spiff_end_of_directory_entry();

        Assert::AreEqual(static_cast<size_t>(10), writer.bytes_written());

        // Verify Entry Magic Number (EMN)
        Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::application_data8), buffer[1]);

        // Verify EOD Entry Length (EOD = End Of Directory)
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[2]);
        Assert::AreEqual(static_cast<uint8_t>(8), buffer[3]);

        // Verify EOD Tag
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[4]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[5]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[6]);
        Assert::AreEqual(static_cast<uint8_t>(1), buffer[7]);

        // Verify embedded SOI tag
        Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[8]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::start_of_image), buffer[9]);
    }

    TEST_METHOD(write_spiff_directory_entry) // NOLINT
    {
        array<uint8_t, 10> buffer{};
        jpeg_stream_writer writer{{buffer.data(), buffer.size()}};

        array<uint8_t, 2> data{0x77, 0x66};

        writer.write_spiff_directory_entry(2, data.data(), data.size());

        // Verify Entry Magic Number (EMN)
        Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[0]);
        Assert::AreEqual(static_cast<uint8_t>(jpeg_marker_code::application_data8), buffer[1]);

        // Verify Entry Length
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[2]);
        Assert::AreEqual(static_cast<uint8_t>(8), buffer[3]);

        // Verify Entry Tag
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[4]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[5]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[6]);
        Assert::AreEqual(static_cast<uint8_t>(2), buffer[7]);

        // Verify embedded data
        Assert::AreEqual(data[0], buffer[8]);
        Assert::AreEqual(data[1], buffer[9]);
    }

    TEST_METHOD(write_start_of_frame_segment) // NOLINT
    {
        constexpr int32_t bits_per_sample{8};
        constexpr int32_t component_count{3};

        array<uint8_t, 19> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        writer.write_start_of_frame_segment(100, UINT16_MAX, bits_per_sample, component_count);

        Assert::AreEqual(static_cast<size_t>(19), writer.bytes_written());

        Assert::AreEqual(static_cast<uint8_t>(0xFF), buffer[0]);
        Assert::AreEqual(static_cast<uint8_t>(0xF7), buffer[1]); // JPEG_SOF_55
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[2]);    // 6 + (3 * 3) + 2 (in big endian)
        Assert::AreEqual(static_cast<uint8_t>(17), buffer[3]);   // 6 + (3 * 3) + 2 (in big endian)
        Assert::AreEqual(static_cast<uint8_t>(bits_per_sample), buffer[4]);
        Assert::AreEqual(static_cast<uint8_t>(255), buffer[5]); // height (in big endian)
        Assert::AreEqual(static_cast<uint8_t>(255), buffer[6]); // height (in big endian)
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[7]);   // width (in big endian)
        Assert::AreEqual(static_cast<uint8_t>(100), buffer[8]); // width (in big endian)
        Assert::AreEqual(static_cast<uint8_t>(component_count), buffer[9]);

        Assert::AreEqual(static_cast<uint8_t>(1), buffer[10]);
        Assert::AreEqual(static_cast<uint8_t>(0x11), buffer[11]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[12]);

        Assert::AreEqual(static_cast<uint8_t>(2), buffer[13]);
        Assert::AreEqual(static_cast<uint8_t>(0x11), buffer[14]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[15]);

        Assert::AreEqual(static_cast<uint8_t>(3), buffer[16]);
        Assert::AreEqual(static_cast<uint8_t>(0x11), buffer[17]);
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[18]);
    }

    TEST_METHOD(write_start_of_frame_marker_segment_with_low_boundary_values) // NOLINT
    {
        constexpr int32_t bits_per_sample{2};
        constexpr int32_t component_count{1};

        array<uint8_t, 13> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        writer.write_start_of_frame_segment(0, 0, bits_per_sample, component_count);

        Assert::AreEqual(buffer.size(), writer.bytes_written());
        Assert::AreEqual(static_cast<uint8_t>(bits_per_sample), buffer[4]);
        Assert::AreEqual(static_cast<uint8_t>(component_count), buffer[9]);
    }

    TEST_METHOD(write_start_of_frame_marker_segment_with_high_boundary_values_and_serialize) // NOLINT
    {
        array<uint8_t, 775> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        writer.write_start_of_frame_segment(UINT16_MAX, UINT16_MAX, 16, UINT8_MAX);

        Assert::AreEqual(buffer.size(), writer.bytes_written());
        Assert::AreEqual(static_cast<uint8_t>(16), buffer[4]);
        Assert::AreEqual(static_cast<uint8_t>(UINT8_MAX), buffer[9]);

        Assert::AreEqual(static_cast<uint8_t>(UINT8_MAX), buffer[buffer.size() - 3]); // Last component index.
    }

    TEST_METHOD(write_color_transform_segment) // NOLINT
    {
        const color_transformation transformation = color_transformation::hp1;

        array<uint8_t, 9> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        writer.write_color_transform_segment(transformation);
        Assert::AreEqual(buffer.size(), writer.bytes_written());

        // Verify mrfx identifier string.
        Assert::AreEqual(static_cast<uint8_t>('m'), buffer[4]);
        Assert::AreEqual(static_cast<uint8_t>('r'), buffer[5]);
        Assert::AreEqual(static_cast<uint8_t>('f'), buffer[6]);
        Assert::AreEqual(static_cast<uint8_t>('x'), buffer[7]);

        Assert::AreEqual(static_cast<uint8_t>(transformation), buffer[8]);
    }

    TEST_METHOD(write_jpegls_extended_parameters_marker_and_serialize) // NOLINT
    {
        const jpegls_pc_parameters presets{2, 1, 2, 3, 7};

        array<uint8_t, 15> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        writer.write_jpegls_preset_parameters_segment(presets);
        Assert::AreEqual(buffer.size(), writer.bytes_written());

        // Parameter ID.
        Assert::AreEqual(static_cast<uint8_t>(0x1), buffer[4]);

        // MaximumSampleValue
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[5]);
        Assert::AreEqual(static_cast<uint8_t>(2), buffer[6]);

        // Threshold1
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[7]);
        Assert::AreEqual(static_cast<uint8_t>(1), buffer[8]);

        // Threshold2
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[9]);
        Assert::AreEqual(static_cast<uint8_t>(2), buffer[10]);

        // Threshold3
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[11]);
        Assert::AreEqual(static_cast<uint8_t>(3), buffer[12]);

        // ResetValue
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[13]);
        Assert::AreEqual(static_cast<uint8_t>(7), buffer[14]);
    }

    TEST_METHOD(write_start_of_scan_marker) // NOLINT
    {
        array<uint8_t, 10> buffer{};
        jpeg_stream_writer writer({buffer.data(), buffer.size()});

        writer.write_start_of_scan_segment(1, 2, interleave_mode::none);

        Assert::AreEqual(buffer.size(), writer.bytes_written());
        Assert::AreEqual(static_cast<uint8_t>(1), buffer[4]); // component count.
        Assert::AreEqual(static_cast<uint8_t>(1), buffer[5]); // component index.
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[6]); // table ID.
        Assert::AreEqual(static_cast<uint8_t>(2), buffer[7]); // NEAR parameter.
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[8]); // ILV parameter.
        Assert::AreEqual(static_cast<uint8_t>(0), buffer[9]); // transformation.
    }
};

}} // namespace charls::test
