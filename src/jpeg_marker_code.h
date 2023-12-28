// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstddef>
#include <cstdint>

namespace charls {

// JPEG Marker codes have the pattern 0xFFaa in a JPEG byte stream.
// The valid 'aa' options are defined by several ISO/IEC, ITU standards:
// 0x00, 0x01, 0xFE, 0xC0-0xDF are defined in ISO/IEC 10918-1, ITU T.81
// 0xF0 - 0xF6 are defined in ISO/IEC 10918-3 | ITU T.84: JPEG extensions
// 0xF7 - 0xF8 are defined in ISO/IEC 14495-1 | ITU T.87: JPEG LS baseline
// 0xF9         is defined in ISO/IEC 14495-2 | ITU T.870: JPEG LS extensions
// 0x4F - 0x6F, 0x90 - 0x93 are defined in ISO/IEC 15444-1: JPEG 2000

constexpr std::byte jpeg_marker_start_byte{0xFF};
constexpr uint8_t jpeg_restart_marker_base{0xD0}; // RSTm: Marks the next restart interval (range is D0 to D7)
constexpr uint32_t jpeg_restart_marker_range{8};

enum class jpeg_marker_code : uint8_t
{
    /// <summary>SOI: Marks the start of an image.</summary>
    start_of_image = 0xD8,

    /// <summary>EOI: Marks the end of an image.</summary>
    end_of_image = 0xD9,

    /// <summary>SOS: Marks the start of scan.</summary>
    start_of_scan = 0xDA,

    /// <summary>DNL: Defines the number of lines in a scan.</summary>
    define_number_of_lines = 0xDC,

    /// <summary>DRI: Defines the restart interval used in succeeding scans.</summary>
    define_restart_interval = 0xDD,


    // The following markers are defined in ISO/IEC 10918-1 | ITU T.81 (general JPEG standard).

    /// <summary>SOF_0: Marks the start of a baseline jpeg encoded frame.</summary>
    start_of_frame_baseline_jpeg = 0xC0,

    /// <summary>SOF_1: Marks the start of an extended sequential Huffman encoded frame.</summary>
    start_of_frame_extended_sequential = 0xC1,

    /// <summary>SOF_2: Marks the start of a progressive Huffman encoded frame.</summary>
    start_of_frame_progressive = 0xC2, //

    /// <summary>SOF_3: Marks the start of a lossless Huffman encoded frame.</summary>
    start_of_frame_lossless = 0xC3,

    /// <summary>SOF_5: Marks the start of a differential sequential Huffman encoded frame.</summary>
    start_of_frame_differential_sequential = 0xC5,

    /// <summary>SOF_6: Marks the start of a differential progressive Huffman encoded frame.</summary>
    start_of_frame_differential_progressive = 0xC6,

    /// <summary>SOF_7: Marks the start of a differential lossless Huffman encoded frame.</summary>
    start_of_frame_differential_lossless = 0xC7,

    /// <summary>SOF_9: Marks the start of an extended sequential arithmetic encoded frame.</summary>
    start_of_frame_extended_arithmetic = 0xC9,

    /// <summary>SOF_10: Marks the start of a progressive arithmetic encoded frame.</summary>
    start_of_frame_progressive_arithmetic = 0xCA,

    /// <summary>SOF_11: Marks the start of a lossless arithmetic encoded frame.</summary>
    start_of_frame_lossless_arithmetic = 0xCB,


    // The following markers are defined in ISO/IEC 14495-1 | ITU T.87. (JPEG-LS standard)

    /// <summary>SOF_55: Marks the start of a JPEG-LS encoded frame.</summary>
    start_of_frame_jpegls = 0xF7,

    /// <summary>LSE: Marks the start of a JPEG-LS preset parameters segment.</summary>
    jpegls_preset_parameters = 0xF8,

    /// <summary>SOF_57: Marks the start of a JPEG-LS extended (ISO/IEC 14495-2) encoded frame.</summary>
    start_of_frame_jpegls_extended = 0xF9,

    /// <summary>APP0: Application data 0: used for JFIF header.</summary>
    application_data0 = 0xE0,

    /// <summary>APP1: Application data 1: used for EXIF or XMP header.</summary>
    application_data1 = 0xE1,

    /// <summary>APP2: Application data 2: used for ICC profile.</summary>
    application_data2 = 0xE2,

    /// <summary>APP3: Application data 3: used for meta info</summary>
    application_data3 = 0xE3,

    /// <summary>APP4: Application data 4.</summary>
    application_data4 = 0xE4,

    /// <summary>APP5: Application data 5.</summary>
    application_data5 = 0xE5,

    /// <summary>APP6: Application data 6.</summary>
    application_data6 = 0xE6,

    /// <summary>APP7: Application data 7: used for HP color-space info.</summary>
    application_data7 = 0xE7,

    /// <summary>APP8: Application data 8: used for HP color-transformation info or SPIFF header.</summary>
    application_data8 = 0xE8,

    /// <summary>APP9: Application data 9.</summary>
    application_data9 = 0xE9,

    /// <summary>APP10: Application data 10.</summary>
    application_data10 = 0xEA,

    /// <summary>APP11: Application data 11.</summary>
    application_data11 = 0xEB,

    /// <summary>APP12: Application data 12: used for Picture info.</summary>
    application_data12 = 0xEC,

    /// <summary>APP13: Application data 13: used by PhotoShop IRB</summary>
    application_data13 = 0xED,

    /// <summary>APP14: Application data 14: used by Adobe</summary>
    application_data14 = 0xEE,

    /// <summary>APP15: Application data 15.</summary>
    application_data15 = 0xEF,

    /// <summary>COM: Comment block.</summary>
    comment = 0xFE
};

} // namespace charls
