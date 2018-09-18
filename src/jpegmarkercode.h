// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

#include <cstdint>

// JPEG Marker codes have the pattern 0xFFaa in a JPEG byte stream.
// The valid 'aa' options are defined by several ITU / IEC standards:
// 0x00, 0x01, 0xFE, 0xC0-0xDF are defined in ITU T.81/IEC 10918-1
// 0xF0 - 0xF6 are defined in ITU T.84/IEC 10918-3 JPEG extensions
// 0xF7 - 0xF8 are defined in ITU T.87/IEC 14495-1 JPEG LS
// 0x4F - 0x6F, 0x90 - 0x93 are defined in JPEG 2000 IEC 15444-1
enum class JpegMarkerCode : uint8_t
{
    StartOfImage = 0xD8, // SOI: Marks the start of an image.
    EndOfImage = 0xD9,   // EOI: Marks the end of an image.
    StartOfScan = 0xDA,  // SOS: Marks the start of scan.

    // The following markers are defined in ITU T.81 | ISO IEC 10918-1.
    StartOfFrameBaselineJpeg = 0xC0,            // SOF_0:  Marks the start of a baseline jpeg encoded frame.
    StartOfFrameExtendedSequential = 0xC1,      // SOF_1:  Marks the start of a extended sequential Huffman encoded frame.
    StartOfFrameProgressive = 0xC2,             // SOF_2:  Marks the start of a progressive Huffman encoded frame.
    StartOfFrameLossless = 0xC3,                // SOF_3:  Marks the start of a lossless Huffman encoded frame.
    StartOfFrameDifferentialSequential = 0xC5,  // SOF_5:  Marks the start of a differential sequential Huffman encoded frame.
    StartOfFrameDifferentialProgressive = 0xC6, // SOF_6:  Marks the start of a differential progressive Huffman encoded frame.
    StartOfFrameDifferentialLossless = 0xC7,    // SOF_7:  Marks the start of a differential lossless Huffman encoded frame.
    StartOfFrameExtendedArithmetic = 0xC9,      // SOF_9:  Marks the start of a extended sequential arithmetic encoded frame.
    StartOfFrameProgressiveArithmetic = 0xCA,   // SOF_10: Marks the start of a progressive arithmetic encoded frame.
    StartOfFrameLosslessArithmetic = 0xCB,      // SOF_11: Marks the start of a lossless arithmetic encoded frame.

    StartOfFrameJpegLS = 0xF7,                  // SOF_55: Marks the start of a JPEG-LS encoded frame.
    JpegLSPresetParameters = 0xF8,              // LSE:    Marks the start of a JPEG-LS preset parameters segment.

    /// <summary>
    /// SOF_57: Marks the start of a JPEG-LS extended (ISO/IEC 14495-2) encoded frame.
    /// </summary>
    StartOfFrameJpegLSExtended = 0xF9,

    ApplicationData0 = 0xE0,                    // APP0: Application data 0: used for JFIF header.
    ApplicationData1 = 0xE1,                    // APP1: Application data 1: used for EXIF or XMP header.
    ApplicationData2 = 0xE2,                    // APP2: Application data 2: used for ICC profile.
    ApplicationData3 = 0xE3,                    // APP3: Application data 3: used for meta info
    ApplicationData4 = 0xE4,                    // APP4: Application data 4.
    ApplicationData5 = 0xE5,                    // APP5: Application data 5.
    ApplicationData6 = 0xE6,                    // APP6: Application data 6.
    ApplicationData7 = 0xE7,                    // APP7: Application data 7: used for HP color-space info.
    ApplicationData8 = 0xE8,                    // APP8: Application data 8: used for HP color-transformation info or SPIFF header.
    ApplicationData9 = 0xE9,                    // APP9: Application data 9.
    ApplicationData10 = 0xEA,                   // APP10: Application data 10.
    ApplicationData11 = 0xEB,                   // APP11: Application data 11.
    ApplicationData12 = 0xEC,                   // APP12: Application data 12: used for Picture info.
    ApplicationData13 = 0xEE,                   // APP13: Application data 13: used by PhotoShop IRB
    ApplicationData14 = 0xED,                   // APP14: Application data 14: used by Adobe
    ApplicationData15 = 0xEF,                   // APP15: Application data 15.
    Comment = 0xFE                              // COM:  Comment block.
};
