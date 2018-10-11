// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include "util.h"

#include "../src/defaulttraits.h"
#include "../src/losslesstraits.h"
#include "../src/processline.h"

#include "bitstreamdamage.h"
#include "compliance.h"
#include "performance.h"
#include "dicomsamples.h"

#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <array>

using namespace charls;

namespace
{

const std::ios_base::openmode mode_input  = std::ios_base::in  | std::ios::binary;
const std::ios_base::openmode mode_output = std::ios_base::out | std::ios::binary;


bool ScanFile(const char* strNameEncoded, std::vector<uint8_t>* rgbyteFile, JlsParameters* params)
{
    if (!ReadFile(strNameEncoded, rgbyteFile))
    {
        Assert::IsTrue(false);
        return false;
    }
    std::basic_filebuf<char> jlsFile;
    jlsFile.open(strNameEncoded, mode_input);

    const ByteStreamInfo rawStreamInfo = {&jlsFile, nullptr, 0};

    const auto err = JpegLsReadHeaderStream(rawStreamInfo, params, nullptr);
    Assert::IsTrue(err == ApiResult::OK);
    return err == ApiResult::OK;
}


void TestTraits16bit()
{
    const auto traits1 = DefaultTraits<uint16_t, uint16_t>(4095,0);
    const auto traits2 = LosslessTraits<uint16_t, 12>();

    Assert::IsTrue(traits1.LIMIT == traits2.LIMIT);
    Assert::IsTrue(traits1.MAXVAL == traits2.MAXVAL);
    Assert::IsTrue(traits1.RESET == traits2.RESET);
    Assert::IsTrue(traits1.bpp == traits2.bpp);
    Assert::IsTrue(traits1.qbpp == traits2.qbpp);

    for (int i = -4096; i < 4096; ++i)
    {
        Assert::IsTrue(traits1.ModuloRange(i) == traits2.ModuloRange(i));
        Assert::IsTrue(traits1.ComputeErrVal(i) == traits2.ComputeErrVal(i));
    }

    for (int i = -8095; i < 8095; ++i)
    {
        Assert::IsTrue(traits1.CorrectPrediction(i) == traits2.CorrectPrediction(i));
        Assert::IsTrue(traits1.IsNear(i,2) == traits2.IsNear(i,2));
    }
}


void TestTraits8bit()
{
    const auto traits1 = DefaultTraits<uint8_t, uint8_t>(255,0);
    const auto traits2 = LosslessTraits<uint8_t, 8>();

    Assert::IsTrue(traits1.LIMIT == traits2.LIMIT);
    Assert::IsTrue(traits1.MAXVAL == traits2.MAXVAL);
    Assert::IsTrue(traits1.RESET == traits2.RESET);
    Assert::IsTrue(traits1.bpp == traits2.bpp);
    Assert::IsTrue(traits1.qbpp == traits2.qbpp);

    for (int i = -255; i < 255; ++i)
    {
        Assert::IsTrue(traits1.ModuloRange(i) == traits2.ModuloRange(i));
        Assert::IsTrue(traits1.ComputeErrVal(i) == traits2.ComputeErrVal(i));
    }

    for (int i = -255; i < 512; ++i)
    {
        Assert::IsTrue(traits1.CorrectPrediction(i) == traits2.CorrectPrediction(i));
        Assert::IsTrue(traits1.IsNear(i,2) == traits2.IsNear(i,2));
    }
}


std::vector<uint8_t> MakeSomeNoise(size_t length, size_t bitcount, int seed)
{
    srand(seed);
    std::vector<uint8_t> rgbyteNoise(length);
    const auto mask = static_cast<uint8_t>((1 << bitcount) - 1);
    for (size_t icol = 0; icol < length; ++icol)
    {
        const auto val = static_cast<uint8_t>(rand());
        rgbyteNoise[icol] = static_cast<uint8_t>(val & mask);
    }
    return rgbyteNoise;
}


std::vector<uint8_t> MakeSomeNoise16bit(size_t length, int bitcount, int seed)
{
    srand(seed);
    std::vector<uint8_t> buffer(length * 2);
    const auto mask = static_cast<uint16_t>((1 << bitcount) - 1);
    for (size_t i = 0; i < length; i = i + 2)
    {
        const uint16_t value = static_cast<uint16_t>(rand()) & mask;

        buffer[i] = static_cast<uint8_t>(value);
        buffer[i] = static_cast<uint8_t>(value >> 8);

    }
    return buffer;
}


void TestNoiseImage()
{
    const Size size2 = Size(512, 512);

    for (size_t bitDepth = 8; bitDepth >=2; --bitDepth)
    {
        std::stringstream label;
        label << "noise, bit depth: " << bitDepth;

        const std::vector<uint8_t> noiseBytes = MakeSomeNoise(size2.cx * size2.cy, bitDepth, 21344);
        TestRoundTrip(label.str().c_str(), noiseBytes, size2, static_cast<int>(bitDepth), 1);
    }

    for (int bitDepth = 16; bitDepth > 8; --bitDepth)
    {
        std::stringstream label;
        label << "noise, bit depth: " << bitDepth;

        const std::vector<uint8_t> noiseBytes = MakeSomeNoise16bit(size2.cx * size2.cy, bitDepth, 21344);
        TestRoundTrip(label.str().c_str(), noiseBytes, size2, bitDepth, 1);
    }
}


void TestNoiseImageWithCustomReset()
{
    const Size size{512, 512};
    const int bitDepth = 16;
    const std::vector<uint8_t> noiseBytes = MakeSomeNoise16bit(size.cx * size.cy, bitDepth, 21344);

    JlsParameters params{};
    params.components = 1;
    params.bitsPerSample = bitDepth;
    params.height = static_cast<int>(size.cy);
    params.width = static_cast<int>(size.cx);
    params.custom.MaximumSampleValue = (1 << bitDepth) - 1;
    params.custom.ResetValue = 63;

    TestRoundTrip("TestNoiseImageWithCustomReset", noiseBytes, params);
}


void TestFailOnTooSmallOutputBuffer()
{
    auto inputBuffer = MakeSomeNoise(8 * 8, 8, 21344);
    size_t compressedLength;

    auto params = JlsParameters();
    params.components = 1;
    params.bitsPerSample = 8;
    params.height = 8;
    params.width = 8;

    // Trigger a "buffer too small"" when writing the header markers.
    std::vector<uint8_t> outputBuffer1(1);
    auto result = JpegLsEncode(outputBuffer1.data(), outputBuffer1.size(), &compressedLength, inputBuffer.data(), inputBuffer.size(), &params, nullptr);
    Assert::IsTrue(result == ApiResult::CompressedBufferTooSmall);

    // Trigger a "buffer too small"" when writing the encoded pixel bytes.
    std::vector<uint8_t> outputBuffer2(100);
    result = JpegLsEncode(outputBuffer2.data(), outputBuffer2.size(), &compressedLength, inputBuffer.data(), inputBuffer.size(), &params, nullptr);
    Assert::IsTrue(result == ApiResult::CompressedBufferTooSmall);
}


void TestBgra()
{
    char rgbyteTest[] = "RGBARGBARGBARGBA1234";
    const char rgbyteComp[] = "BGRABGRABGRABGRA1234";
    TransformRgbToBgr(rgbyteTest, 4, 4);
    Assert::IsTrue(strcmp(rgbyteTest, rgbyteComp) == 0);
}


void TestBgr()
{
    JlsParameters params{};
    std::vector<uint8_t> rgbyteEncoded;
    ScanFile("test/conformance/T8C2E3.JLS", &rgbyteEncoded, &params);
    std::vector<uint8_t> rgbyteDecoded(static_cast<size_t>(params.width) * params.height * params.components);

    params.outputBgr = static_cast<char>(true);

    const auto err = JpegLsDecode(&rgbyteDecoded[0], rgbyteDecoded.size(), &rgbyteEncoded[0], rgbyteEncoded.size(), &params, nullptr);
    Assert::IsTrue(err == ApiResult::OK);

    Assert::IsTrue(rgbyteDecoded[0] == 0x69);
    Assert::IsTrue(rgbyteDecoded[1] == 0x77);
    Assert::IsTrue(rgbyteDecoded[2] == 0xa1);
    Assert::IsTrue(rgbyteDecoded[static_cast<size_t>(params.width) * 6 + 3] == 0x2d);
    Assert::IsTrue(rgbyteDecoded[static_cast<size_t>(params.width) * 6 + 4] == 0x43);
    Assert::IsTrue(rgbyteDecoded[static_cast<size_t>(params.width) * 6 + 5] == 0x4d);
}


void TestTooSmallOutputBuffer()
{
    std::vector<uint8_t> rgbyteCompressed;
    if (!ReadFile("test/lena8b.jls", &rgbyteCompressed, 0))
        return;

    std::vector<uint8_t> rgbyteOut(512 * 511);
    const auto error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], rgbyteCompressed.size(), nullptr, nullptr);

    Assert::IsTrue(error == ApiResult::UncompressedBufferTooSmall);
}


////void TestBadImage()
////{
////    std::vector<uint8_t> rgbyteCompressed;
////    if (!ReadFile("test/BadCompressedStream.jls", &rgbyteCompressed, 0))
////        return;
////
////    std::vector<uint8_t> rgbyteOut(2500 * 3000 * 2);
////    auto error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], rgbyteCompressed.size(), nullptr, nullptr);
////
////    Assert::IsTrue(error == ApiResult::UncompressedBufferTooSmall);
////}


void TestDecodeBitStreamWithNoMarkerStart()
{
    const std::array<uint8_t, 2> encodedData = {0x33, 0x33};
    std::array<uint8_t, 1000> output{};

    const auto error = JpegLsDecode(output.data(), output.size(), encodedData.data(), encodedData.size(), nullptr, nullptr);
    Assert::IsTrue(error == ApiResult::MissingJpegMarkerStart);
}


void TestDecodeBitStreamWithUnsupportedEncoding()
{
    const std::array<uint8_t, 6> encodedData = {
        0xFF, 0xD8, // Start Of Image (JPEG_SOI)
        0xFF, 0xC3, // Start Of Frame (lossless, Huffman) (JPEG_SOF_3)
        0x00, 0x00  // Length of data of the marker
    };
    std::array<uint8_t, 1000> output{};

    const auto error = JpegLsDecode(output.data(), output.size(), encodedData.data(), encodedData.size(), nullptr, nullptr);
    Assert::IsTrue(error == ApiResult::UnsupportedEncoding);
}


void TestDecodeBitStreamWithUnknownJpegMarker()
{
    const std::array<uint8_t, 6> encodedData = {
        0xFF, 0xD8, // Start Of Image (JPEG_SOI)
        0xFF, 0x01, // Undefined marker
        0x00, 0x00  // Length of data of the marker
    };
    std::array<uint8_t, 1000> output{};

    const auto error = JpegLsDecode(output.data(), output.size(), encodedData.data(), encodedData.size(), nullptr, nullptr);
    Assert::IsTrue(error == ApiResult::UnknownJpegMarker);
}


void TestDecodeRect()
{
    std::vector<uint8_t> rgbyteCompressed;
    JlsParameters params{};
    if (!ScanFile("test/lena8b.jls", &rgbyteCompressed, &params))
        return;

    std::vector<uint8_t> rgbyteOutFull(static_cast<size_t>(params.width) * params.height*params.components);
    auto error = JpegLsDecode(&rgbyteOutFull[0], rgbyteOutFull.size(), &rgbyteCompressed[0], rgbyteCompressed.size(), nullptr, nullptr);
    Assert::IsTrue(error == ApiResult::OK);

    const JlsRect rect = { 128, 128, 256, 1 };
    std::vector<uint8_t> rgbyteOut(static_cast<size_t>(rect.Width) * rect.Height);
    rgbyteOut.push_back(0x1f);
    error = JpegLsDecodeRect(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], rgbyteCompressed.size(), rect, nullptr, nullptr);
    Assert::IsTrue(error == ApiResult::OK);

    Assert::IsTrue(memcmp(&rgbyteOutFull[rect.X + static_cast<size_t>(rect.Y) * 512], &rgbyteOut[0], static_cast<size_t>(rect.Width) * rect.Height) == 0);
    Assert::IsTrue(rgbyteOut[static_cast<size_t>(rect.Width) * rect.Height] == 0x1f);
}


void TestEncodeFromStream(const char* file, int offset, int width, int height, int bpp, int ccomponent, InterleaveMode ilv, size_t expectedLength)
{
    std::basic_filebuf<char> myFile; // On the stack
    myFile.open(file, mode_input);
    Assert::IsTrue(myFile.is_open());

    myFile.pubseekoff(static_cast<std::streamoff>(offset), std::ios_base::cur);
    const ByteStreamInfo rawStreamInfo = {&myFile, nullptr, 0};

    std::vector<uint8_t> compressed(static_cast<size_t>(width) * height * ccomponent * 2);
    JlsParameters params = JlsParameters();
    params.height = height;
    params.width = width;
    params.components = ccomponent;
    params.bitsPerSample = bpp;
    params.interleaveMode = ilv;
    size_t bytesWritten = 0;

    JpegLsEncodeStream(FromByteArray(compressed.data(), static_cast<size_t>(width) * height * ccomponent * 2), bytesWritten, rawStreamInfo, params, nullptr);
    Assert::IsTrue(bytesWritten == expectedLength);

    myFile.close();
}


bool DecodeToPnm(std::istream& input, std::ostream& output)
{
    const ByteStreamInfo inputInfo{input.rdbuf(), nullptr, 0};

    auto params = JlsParameters();
    auto result = JpegLsReadHeaderStream(inputInfo, &params, nullptr);
    if (result != ApiResult::OK)
        return false;
    input.seekg(0);

    const int maxValue = (1 << params.bitsPerSample) - 1;
    const int bytesPerSample = maxValue > 255 ? 2 : 1;
    std::vector<uint8_t> outputBuffer(static_cast<size_t>(params.width) * params.height * bytesPerSample * params.components);
    const auto outputInfo = FromByteArray(outputBuffer.data(), outputBuffer.size());
    result = JpegLsDecodeStream(outputInfo, inputInfo, &params, nullptr);
    if (result != ApiResult::OK)
        return false;

    // PNM format requires most significant byte first (big endian).
    if (bytesPerSample == 2)
    {
        for (auto i = outputBuffer.begin(); i != outputBuffer.end(); i += 2)
        {
            std::iter_swap(i, i + 1);
        }
    }

    const int magicNumber = params.components == 3 ? 6 : 5;
    output << 'P' << magicNumber << std::endl << params.width << ' ' << params.height << std::endl << maxValue << std::endl;
    output.write(reinterpret_cast<char*>(outputBuffer.data()), outputBuffer.size());

    return true;
}


std::vector<int> readPnmHeader(std::istream& pnmFile)
{
    std::vector<int> readValues;

    const auto first = static_cast<char>(pnmFile.get());

    // All portable anymap format (PNM) start with the character P.
    if (first != 'P')
        return readValues;

    while (readValues.size() < 4)
    {
        std::string bytes;
        std::getline(pnmFile, bytes);
        std::stringstream line(bytes);

        while (readValues.size() < 4)
        {
            int value = -1;
            line >> value;
            if (value <= 0)
                break;

            readValues.push_back(value);
        }
    }
    return readValues;
}


// Purpose: this function can encode an image stored in the Portable Anymap Format (PNM)
//          into the JPEG-LS format. The 2 binary formats P5 and P6 are supported:
//          Portable GrayMap: P5 = binary, extension = .pgm, 0-2^16 (gray scale)
//          Portable PixMap: P6 = binary, extension.ppm, range 0-2^16 (RGB)
bool EncodePnm(std::istream& pnmFile, const std::ostream& jlsFileStream)
{
    std::vector<int> readValues = readPnmHeader(pnmFile);
    if (readValues.size() !=4)
        return false;

    JlsParameters params{};
    params.components = readValues[0] == 6 ? 3 : 1;
    params.width = readValues[1];
    params.height = readValues[2];
    params.bitsPerSample = log_2(readValues[3] + 1);
    params.interleaveMode = params.components == 3 ? InterleaveMode::Line : InterleaveMode::None;

    const int bytesPerSample = (params.bitsPerSample + 7) / 8;
    std::vector<uint8_t> inputBuffer(static_cast<size_t>(params.width) * params.height * bytesPerSample * params.components);
    pnmFile.read(reinterpret_cast<char*>(inputBuffer.data()), inputBuffer.size());
    if (!pnmFile.good())
        return false;

    // PNM format is stored with most significant byte first (big endian).
    if (bytesPerSample == 2)
    {
        for (auto i = inputBuffer.begin(); i != inputBuffer.end(); i += 2)
        {
            std::iter_swap(i, i + 1);
        }
    }

    const auto rawStreamInfo = FromByteArray(inputBuffer.data(), inputBuffer.size());
    const ByteStreamInfo jlsStreamInfo = {jlsFileStream.rdbuf(), nullptr, 0};

    size_t bytesWritten = 0;
    JpegLsEncodeStream(jlsStreamInfo, bytesWritten, rawStreamInfo, params, nullptr);
    return true;
}


bool ComparePnm(std::istream& pnmFile1, std::istream& pnmFile2)
{
    std::vector<int> header1 = readPnmHeader(pnmFile1);
    if (header1.size() != 4)
    {
        printf("Cannot read header from input file 1\r\n");
        return false;
    }

    std::vector<int> header2 = readPnmHeader(pnmFile2);
    if (header2.size() != 4)
    {
        printf("Cannot read header from input file 2\r\n");
        return false;
    }

    if (header1[0] != header2[0])
    {
        printf("Header type %i is not equal with type %i\r\n", header1[0], header2[0]);
        return false;
    }

    const size_t width = header1[1];
    if (width != static_cast<size_t>(header2[1]))
    {
        printf("Width %zu is not equal with width %i\r\n", width, header2[1]);
        return false;
    }

    const size_t height = header1[2];
    if (height != static_cast<size_t>(header2[2]))
    {
        printf("Height %zu is not equal with height %i\r\n", height, header2[2]);
        return false;
    }

    if (header1[3] != header2[3])
    {
        printf("max-value %i is not equal with max-value %i\r\n", header1[3], header2[3]);
        return false;
    }
    const auto bytesPerSample = header1[3] > 255 ? 2 : 1;

    const size_t byteCount = width * height * bytesPerSample;
    std::vector<uint8_t> bytes1(byteCount);
    std::vector<uint8_t> bytes2(byteCount);

    pnmFile1.read(reinterpret_cast<char*>(&bytes1[0]), byteCount);
    pnmFile2.read(reinterpret_cast<char*>(&bytes2[0]), byteCount);

    for (size_t x = 0; x < height; ++x)
    {
        for (size_t y = 0; y < width; y += bytesPerSample)
        {
            if (bytesPerSample == 1)
            {
                if (bytes1[(x * width) + y] != bytes2[(x * width) + y])
                {
                    printf("Values of the 2 files are different, height:%zu, width:%zu\r\n", x, y);
                    return false;
                }
            }
            else
            {
                if (bytes1[(x * width) + y] != bytes2[(x * width) + y] ||
                    bytes1[(x * width) + (y + 1)] != bytes2[(x * width) + (y + 1)])
                {
                    printf("Values of the 2 files are different, height:%zu, width:%zu\r\n", x, y);
                    return false;
                }
            }
        }
    }

    printf("Values of the 2 files are equal\r\n");
    return true;
}


////void TestDecodeFromStream(const char* strNameEncoded)
////{
////    std::basic_filebuf<char> jlsFile;
////    jlsFile.open(strNameEncoded, mode_input);
////    Assert::IsTrue(jlsFile.is_open());
////    ByteStreamInfo compressedByteStream = {&jlsFile, nullptr, 0};
////
////    auto params = JlsParameters();
////    auto err = JpegLsReadHeaderStream(compressedByteStream, &params, nullptr);
////    Assert::IsTrue(err == ApiResult::OK);
////
////    jlsFile.pubseekpos(std::ios::beg, std::ios_base::in);
////
////    std::basic_stringbuf<char> buf;
////    ByteStreamInfo rawStreamInfo = {&buf, nullptr, 0};
////
////    err = JpegLsDecodeStream(rawStreamInfo, compressedByteStream, nullptr, nullptr);
////    ////size_t outputCount = buf.str().size();
////
////    Assert::IsTrue(err == ApiResult::OK);
////    //Assert::IsTrue(outputCount == 512 * 512);
////}


ApiResult DecodeRaw(const char* strNameEncoded, const char* strNameOutput)
{
    std::fstream jlsFile(strNameEncoded, mode_input);
    const ByteStreamInfo compressedByteStream{jlsFile.rdbuf(), nullptr, 0};

    std::fstream rawFile(strNameOutput, mode_output);
    const ByteStreamInfo rawStream{rawFile.rdbuf(), nullptr, 0};

    const auto value = JpegLsDecodeStream(rawStream, compressedByteStream, nullptr, nullptr);
    jlsFile.close();
    rawFile.close();

    return value;
}


void TestEncodeFromStream()
{
    ////TestDecodeFromStream("test/user_supplied/output.jls");

    TestEncodeFromStream("test/0015.raw", 0, 1024, 1024, 8, 1, InterleaveMode::None, 0x3D3ee);
    ////TestEncodeFromStream("test/MR2_UNC", 1728, 1024, 1024, 16, 1,0, 0x926e1);
    TestEncodeFromStream("test/conformance/TEST8.PPM", 15, 256, 256, 8, 3, InterleaveMode::Sample, 99734);
    TestEncodeFromStream("test/conformance/TEST8.PPM", 15, 256, 256, 8, 3, InterleaveMode::Line, 100615);
}


void UnitTest()
{
    try
    {
        //// TestBadImage();

        printf("Test Conformance\r\n");
        TestEncodeFromStream();
        TestConformance();

        TestDecodeRect();

        printf("Test Traits\r\n");
        TestTraits16bit();
        TestTraits8bit();

        printf("Windows bitmap BGR/BGRA output\r\n");
        TestBgr();
        TestBgra();

        printf("Test Small buffer\r\n");
        TestTooSmallOutputBuffer();

        TestFailOnTooSmallOutputBuffer();

        printf("Test Color transform equivalence on HP images\r\n");
        TestColorTransforms_HpImages();

        printf("Test Annex H3\r\n");
        TestSampleAnnexH3();

        TestNoiseImage();
        TestNoiseImageWithCustomReset();

        printf("Test robustness\r\n");
        TestDecodeBitStreamWithNoMarkerStart();
        TestDecodeBitStreamWithUnsupportedEncoding();
        TestDecodeBitStreamWithUnknownJpegMarker();
    }
    catch (const UnitTestException&)
    {
        std::cout << "==> Unit test failed <==\n";
    }
}

} // namespace


int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        printf("CharLS test runner.\r\nOptions: -unittest, -bitstreamdamage, -performance[:loop-count], -decodeperformance[:loop-count], -dontwait -decoderaw -encodepnm -decodetopnm -comparepnm\r\n");
        return EXIT_FAILURE;
    }

    bool wait = true;
    for (int i = 1; i < argc; ++i)
    {
        std::string str = argv[i];
        if (str == "-unittest")
        {
            UnitTest();
            continue;
        }

        if (str == "-decoderaw")
        {
            if (i != 1 || argc != 4)
            {
                printf("Syntax: -decoderaw inputfile outputfile \r\n");
                return EXIT_FAILURE;
            }
            return DecodeRaw(argv[2], argv[3]) == ApiResult::OK ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        if (str == "-decodetopnm")
        {
            if (i != 1 || argc != 4)
            {
                printf("Syntax: -decodetopnm inputfile outputfile \r\n");
                return 0;
            }
            std::fstream pnmFile(argv[3], mode_output);
            std::fstream jlsFile(argv[2], mode_input);

            return DecodeToPnm(jlsFile, pnmFile) ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        if (str == "-encodepnm")
        {
            if (i != 1 || argc != 4)
            {
                printf("Syntax: -encodepnm inputfile outputfile\r\n");
                return 0;
            }
            std::fstream pnmFile(argv[2], mode_input);
            const std::fstream jlsFile(argv[3], mode_output);

            return EncodePnm(pnmFile, jlsFile) ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        if (str == "-comparepnm")
        {
            if (i != 1 || argc != 4)
            {
                printf("Syntax: -encodepnm inputfile outputfile\r\n");
                return EXIT_FAILURE;
            }
            std::fstream pnmFile1(argv[2], mode_input);
            std::fstream pnmFile2(argv[3], mode_input);

            return ComparePnm(pnmFile1, pnmFile2) ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        if (str == "-bitstreamdamage")
        {
            DamagedBitstreamTests();
            continue;
        }

        if (str.compare(0, 12, "-performance") == 0)
        {
            int loopCount = 1;

            // Extract the optional loop count from the command line. Longer running tests make the measurements more reliable.
            auto index = str.find(':');
            if (index != std::string::npos)
            {
                loopCount = std::stoi(str.substr(++index));
                if (loopCount < 1)
                {
                    printf("Loop count not understood or invalid: %s\r\n", str.c_str());
                    break;
                }
            }

            PerformanceTests(loopCount);
            continue;
        }

        if (str.compare(0, 17, "-rgb8_performance") == 0)
        {
            // See the comments in function, how to prepare this test.
            TestLargeImagePerformanceRgb8(1);
            continue;
        }

        if (str.compare(0, 18, "-decodeperformance") == 0)
        {
            int loopCount = 1;

            // Extract the optional loop count from the command line. Longer running tests make the measurements more reliable.
            auto index = str.find(':');
            if (index != std::string::npos)
            {
                loopCount = std::stoi(str.substr(++index));
                if (loopCount < 1)
                {
                    printf("Loop count not understood or invalid: %s\r\n", str.c_str());
                    break;
                }
            }

            DecodePerformanceTests(loopCount);
            continue;
        }

        if (str == "-dicom")
        {
            TestDicomWG4Images();
            continue;
        }

        if (str == "-dontwait")
        {
            wait = false;
            continue;
        }

        printf("Option not understood: %s\r\n", argv[i]);
        break;
    }

    if (wait)
    {
        std::cout << "Press any key + 'enter' to exit program\n";

        char c;
        std::cin >> c;
    }

    return EXIT_SUCCESS;
}
