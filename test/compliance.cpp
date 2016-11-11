//
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use.
//

#include "compliance.h"
#include "config.h"
#include "util.h"

#include "../src/charls.h"

#include <iostream>
#include <vector>
#include <cstring>

using namespace charls;

namespace
{

void Triplet2Planar(std::vector<BYTE>& rgbyte, Size size)
{
    std::vector<BYTE> rgbytePlanar(rgbyte.size());

    int cbytePlane = size.cx * size.cy;
    for (int index = 0; index < cbytePlane; index++)
    {
        rgbytePlanar[index]                 = rgbyte[index * 3 + 0];
        rgbytePlanar[index + 1*cbytePlane]  = rgbyte[index * 3 + 1];
        rgbytePlanar[index + 2*cbytePlane]  = rgbyte[index * 3 + 2];
    }
    std::swap(rgbyte, rgbytePlanar);
}


bool VerifyEncodedBytes(const void* uncompressedData, size_t uncompressedLength, const void* compressedData, size_t compressedLength)
{
    JlsParameters info = JlsParameters();
    auto error = JpegLsReadHeader(compressedData, compressedLength, &info, nullptr);
    if (error != ApiResult::OK)
        return false;

    std::vector<uint8_t> ourEncodedBytes(compressedLength + 16);
    size_t bytesWriten;
    error = JpegLsEncode(ourEncodedBytes.data(), ourEncodedBytes.size(), &bytesWriten, uncompressedData, uncompressedLength, &info, nullptr);

    for (size_t i = 0; i < compressedLength; ++i)
    {
        if ((reinterpret_cast<const uint8_t*>(compressedData)[i]) != ourEncodedBytes[i])
        {
            return false;
        }
    }

    return true;
}


void TestCompliance(const BYTE* compressedBytes, size_t compressedLength, const BYTE* rgbyteRaw, size_t cbyteRaw, bool bcheckEncode)
{
    JlsParameters info = JlsParameters();
    auto err = JpegLsReadHeader(compressedBytes, compressedLength, &info, nullptr);
    Assert::IsTrue(err == ApiResult::OK);

    if (bcheckEncode)
    {
        Assert::IsTrue(VerifyEncodedBytes(rgbyteRaw, cbyteRaw, compressedBytes, compressedLength));
    }

    std::vector<BYTE> rgbyteOut(info.height *info.width * ((info.bitsPerSample + 7) / 8) * info.components);

    err = JpegLsDecode(rgbyteOut.data(), rgbyteOut.size(), compressedBytes, compressedLength, nullptr, nullptr);
    Assert::IsTrue(err == ApiResult::OK);

    if (info.allowedLossyError == 0)
    {
        for (size_t i = 0; i < cbyteRaw; ++i)
        {
            if (rgbyteRaw[i] != rgbyteOut[i])
            {
                Assert::IsTrue(false);
                break;
            }
        }
    }
}


void DecompressFile(SZC strNameEncoded, SZC strNameRaw, int ioffs, bool bcheckEncode = true)
{
    std::cout << "Conformance test:" << strNameEncoded << "\n\r";
    std::vector<BYTE> rgbyteFile;
    if (!ReadFile(strNameEncoded, &rgbyteFile))
        return;

    JlsParameters params;
    if (JpegLsReadHeader(rgbyteFile.data(), rgbyteFile.size(), &params, nullptr) != ApiResult::OK)
    {
        Assert::IsTrue(false);
        return;
    }

    std::vector<BYTE> rgbyteRaw;
    if (!ReadFile(strNameRaw, &rgbyteRaw, ioffs))
        return;

    if (params.bitsPerSample > 8)
    {
        FixEndian(&rgbyteRaw, false);
    }

    if (params.interleaveMode == InterleaveMode::None && params.components == 3)
    {
        Triplet2Planar(rgbyteRaw, Size(params.width, params.height));
    }

    TestCompliance(rgbyteFile.data(), rgbyteFile.size(), rgbyteRaw.data(), rgbyteRaw.size(), bcheckEncode);
}


////BYTE palettisedDataH10[] = {
////    0xFF, 0xD8, //Start of image (SOI) marker 
////    0xFF, 0xF7, //Start of JPEG-LS frame (SOF 55) marker – marker segment follows 
////    0x00, 0x0B, //Length of marker segment = 11 bytes including the length field 
////    0x02, //P = Precision = 2 bits per sample 
////    0x00, 0x04, //Y = Number of lines = 4 
////    0x00, 0x03, //X = Number of columns = 3 
////    0x01, //Nf = Number of components in the frame = 1 
////    0x01, //C1  = Component ID = 1 (first and only component) 
////    0x11, //Sub-sampling: H1 = 1, V1 = 1 
////    0x00, //Tq1 = 0 (this field is always 0) 
////
////    0xFF, 0xF8, //LSE – JPEG-LS preset parameters marker 
////    0x00, 0x11, //Length of marker segment = 17 bytes including the length field 
////    0x02, //ID = 2, mapping table  
////    0x05, //TID = 5 Table identifier (arbitrary) 
////    0x03, //Wt = 3 Width of table entry 
////    0xFF, 0xFF, 0xFF, //Entry for index 0 
////    0xFF, 0x00, 0x00, //Entry for index 1 
////    0x00, 0xFF, 0x00, //Entry for index 2 
////    0x00, 0x00, 0xFF, //Entry for index 3 
////
////    0xFF, 0xDA, //Start of scan (SOS) marker 
////    0x00, 0x08, //Length of marker segment = 8 bytes including the length field 
////    0x01, //Ns = Number of components for this scan = 1 
////    0x01, //C1 = Component ID = 1  
////    0x05, //Tm 1  = Mapping table identifier = 5 
////    0x00, //NEAR = 0 (near-lossless max error) 
////    0x00, //ILV = 0 (interleave mode = non-interleaved) 
////    0x00, //Al = 0, Ah = 0 (no point transform) 
////    0xDB, 0x95, 0xF0, //3 bytes of compressed image data 
////    0xFF, 0xD9 //End of image (EOI) marker 
////};


const BYTE rgbyte[] = { 0,   0,  90,  74, 
68,  50,  43, 205, 
64, 145, 145, 145, 
100, 145, 145, 145};
////const BYTE rgbyteComp[] =   {   0xFF, 0xD8, 0xFF, 0xF7, 0x00, 0x0B, 0x08, 0x00, 0x04, 0x00, 0x04, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 
////0xC0, 0x00, 0x00, 0x6C, 0x80, 0x20, 0x8E,
////0x01, 0xC0, 0x00, 0x00, 0x57, 0x40, 0x00, 0x00, 0x6E, 0xE6, 0x00, 0x00, 0x01, 0xBC, 0x18, 0x00,
////0x00, 0x05, 0xD8, 0x00, 0x00, 0x91, 0x60, 0xFF, 0xD9};

} // namespace


void TestSampleAnnexH3()
{
    ////Size size = Size(4,4);
    std::vector<BYTE> vecRaw(16);
    memcpy(vecRaw.data(), rgbyte, 16);
    ////  TestJls(vecRaw, size, 8, 1, ILV_NONE, rgbyteComp, sizeof(rgbyteComp), false);
}


void TestColorTransforms_HpImages()
{
    DecompressFile("test/jlsimage/banny_normal.jls", "test/jlsimage/banny.ppm", 38, false);
    DecompressFile("test/jlsimage/banny_HP1.jls", "test/jlsimage/banny.ppm", 38, false);
    DecompressFile("test/jlsimage/banny_HP2.jls", "test/jlsimage/banny.ppm", 38, false);
    DecompressFile("test/jlsimage/banny_HP3.jls", "test/jlsimage/banny.ppm", 38, false);
}


void TestConformance()
{
    // Test 1
    DecompressFile("test/conformance/T8C0E0.JLS", "test/conformance/TEST8.PPM",15);

    // Test 2
    DecompressFile("test/conformance/T8C1E0.JLS", "test/conformance/TEST8.PPM",15);

    // Test 3
    DecompressFile("test/conformance/T8C2E0.JLS", "test/conformance/TEST8.PPM", 15);


    // Test 4
    DecompressFile("test/conformance/T8C0E3.JLS", "test/conformance/TEST8.PPM",15);

    // Test 5
    DecompressFile("test/conformance/T8C1E3.JLS", "test/conformance/TEST8.PPM",15);

    // Test 6
    DecompressFile("test/conformance/T8C2E3.JLS", "test/conformance/TEST8.PPM",15);


    // Test 7
    // Test 8

    // Test 9
    DecompressFile("test/conformance/T8NDE0.JLS", "test/conformance/TEST8BS2.PGM",15);

    // Test 10
    DecompressFile("test/conformance/T8NDE3.JLS", "test/conformance/TEST8BS2.PGM",15);

    // Test 11
    DecompressFile("test/conformance/T16E0.JLS", "test/conformance/TEST16.PGM",16);

    // Test 12
    DecompressFile("test/conformance/T16E3.JLS", "test/conformance/TEST16.PGM",16);

    // additional, Lena compressed with other codec (UBC?), vfy with CharLS
    DecompressFile("test/lena8b.jls", "test/lena8b.raw",0);
}
