// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "util.h"

#include "../src/default_traits.h"
#include "../src/lossless_traits.h"
#include "../src/process_line.h"

#include "bitstreamdamage.h"
#include "compliance.h"
#include "performance.h"
#include "dicomsamples.h"

#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <array>
#include <string>

using std::cout;
using std::ios;
using std::ios_base;
using std::error_code;
using std::array;
using std::vector;
using std::basic_filebuf;
using std::streamoff;
using std::stringstream;
using std::istream;
using std::ostream;
using std::fstream;
using std::string;
using std::iter_swap;
using std::getline;
using namespace charls;

namespace
{

constexpr ios_base::openmode mode_input  = ios_base::in  | ios::binary;
constexpr ios_base::openmode mode_output = ios_base::out | ios::binary;


vector<uint8_t> ScanFile(const char* strNameEncoded, JlsParameters* params)
{
    vector<uint8_t> buffer = ReadFile(strNameEncoded);

    basic_filebuf<char> jlsFile;
    jlsFile.open(strNameEncoded, mode_input);

    const ByteStreamInfo rawStreamInfo {&jlsFile, nullptr, 0};

    const error_code error = JpegLsReadHeaderStream(rawStreamInfo, params);
    if (error)
        throw UnitTestException();

    return buffer;
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


vector<uint8_t> MakeSomeNoise(size_t length, size_t bitCount, int seed)
{
    srand(seed);
    vector<uint8_t> buffer(length);
    const auto mask = static_cast<uint8_t>((1 << bitCount) - 1);
    for (size_t i = 0; i < length; ++i)
    {
        const auto val = static_cast<uint8_t>(rand());
        buffer[i] = static_cast<uint8_t>(val & mask);
    }
    return buffer;
}


vector<uint8_t> MakeSomeNoise16bit(size_t length, int bitCount, int seed)
{
    srand(seed);
    vector<uint8_t> buffer(length * 2);
    const auto mask = static_cast<uint16_t>((1 << bitCount) - 1);
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
        stringstream label;
        label << "noise, bit depth: " << bitDepth;

        const vector<uint8_t> noiseBytes = MakeSomeNoise(size2.cx * size2.cy, bitDepth, 21344);
        TestRoundTrip(label.str().c_str(), noiseBytes, size2, static_cast<int>(bitDepth), 1);
    }

    for (int bitDepth = 16; bitDepth > 8; --bitDepth)
    {
        stringstream label;
        label << "noise, bit depth: " << bitDepth;

        const vector<uint8_t> noiseBytes = MakeSomeNoise16bit(size2.cx * size2.cy, bitDepth, 21344);
        TestRoundTrip(label.str().c_str(), noiseBytes, size2, bitDepth, 1);
    }
}


void TestNoiseImageWithCustomReset()
{
    const Size size{512, 512};
    constexpr int bitDepth = 16;
    const vector<uint8_t> noiseBytes = MakeSomeNoise16bit(size.cx * size.cy, bitDepth, 21344);

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

    // Trigger a "destination buffer too small" when writing the header markers.
    vector<uint8_t> outputBuffer1(1);
    auto result = JpegLsEncode(outputBuffer1.data(), outputBuffer1.size(), &compressedLength, inputBuffer.data(), inputBuffer.size(), &params, nullptr);
    Assert::IsTrue(result == jpegls_errc::destination_buffer_too_small);

    // Trigger a "destination buffer too small" when writing the encoded pixel bytes.
    vector<uint8_t> outputBuffer2(100);
    result = JpegLsEncode(outputBuffer2.data(), outputBuffer2.size(), &compressedLength, inputBuffer.data(), inputBuffer.size(), &params, nullptr);
    Assert::IsTrue(result == jpegls_errc::destination_buffer_too_small);
}


void TestBgra()
{
    char input[] = "RGBARGBARGBARGBA1234";
    const char expected[] = "BGRABGRABGRABGRA1234";
    TransformRgbToBgr(input, 4, 4);
    Assert::IsTrue(strcmp(input, expected) == 0);
}


void TestBgr()
{
    JlsParameters params{};
    vector<uint8_t> encodedBuffer = ScanFile("test/conformance/T8C2E3.JLS", &params);
    vector<uint8_t> decodedBuffer(static_cast<size_t>(params.width) * params.height * params.components);

    params.outputBgr = static_cast<char>(true);

    const error_code error = JpegLsDecode(decodedBuffer.data(), decodedBuffer.size(), encodedBuffer.data(), encodedBuffer.size(), &params, nullptr);
    Assert::IsTrue(!error);

    Assert::IsTrue(decodedBuffer[0] == 0x69);
    Assert::IsTrue(decodedBuffer[1] == 0x77);
    Assert::IsTrue(decodedBuffer[2] == 0xa1);
    Assert::IsTrue(decodedBuffer[static_cast<size_t>(params.width) * 6 + 3] == 0x2d);
    Assert::IsTrue(decodedBuffer[static_cast<size_t>(params.width) * 6 + 4] == 0x43);
    Assert::IsTrue(decodedBuffer[static_cast<size_t>(params.width) * 6 + 5] == 0x4d);
}


void TestTooSmallOutputBuffer()
{
    vector<uint8_t> encoded = ReadFile("test/lena8b.jls");

    vector<uint8_t> destination(512 * 511);
    const auto error = JpegLsDecode(destination.data(), destination.size(), encoded.data(), encoded.size(), nullptr, nullptr);

    Assert::IsTrue(error == jpegls_errc::destination_buffer_too_small);
}


////void TestBadImage()
////{
////    vector<uint8_t> rgbyteCompressed;
////    if (!ReadFile("test/BadCompressedStream.jls", &rgbyteCompressed, 0))
////        return;
////
////    vector<uint8_t> rgbyteOut(2500 * 3000 * 2);
////    auto error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], rgbyteCompressed.size(), nullptr, nullptr);
////
////    Assert::IsTrue(error == jpegls_errc::UncompressedBufferTooSmall);
////}


void TestDecodeBitStreamWithNoMarkerStart()
{
    const array<uint8_t, 2> encodedData = {0x33, 0x33};
    array<uint8_t, 1000> output{};

    const auto error = JpegLsDecode(output.data(), output.size(), encodedData.data(), encodedData.size(), nullptr, nullptr);
    Assert::IsTrue(error == jpegls_errc::jpeg_marker_start_byte_not_found);
}


void TestDecodeBitStreamWithUnsupportedEncoding()
{
    const array<uint8_t, 6> encodedData = {
        0xFF, 0xD8, // Start Of Image (JPEG_SOI)
        0xFF, 0xC3, // Start Of Frame (lossless, Huffman) (JPEG_SOF_3)
        0x00, 0x00  // Length of data of the marker
    };
    array<uint8_t, 1000> output{};

    const auto error = JpegLsDecode(output.data(), output.size(), encodedData.data(), encodedData.size(), nullptr, nullptr);
    Assert::IsTrue(error == jpegls_errc::encoding_not_supported);
}


void TestDecodeBitStreamWithUnknownJpegMarker()
{
    const array<uint8_t, 6> encodedData = {
        0xFF, 0xD8, // Start Of Image (JPEG_SOI)
        0xFF, 0x01, // Undefined marker
        0x00, 0x00  // Length of data of the marker
    };
    array<uint8_t, 1000> output{};

    const auto error = JpegLsDecode(output.data(), output.size(), encodedData.data(), encodedData.size(), nullptr, nullptr);
    Assert::IsTrue(error == jpegls_errc::unknown_jpeg_marker_found);
}


void TestDecodeRect()
{
    JlsParameters params{};
    vector<uint8_t> encodedData = ScanFile("test/lena8b.jls", &params);
    vector<uint8_t> decodedBuffer(static_cast<size_t>(params.width) * params.height*params.components);

    error_code error = JpegLsDecode(decodedBuffer.data(), decodedBuffer.size(), encodedData.data(), encodedData.size(), nullptr, nullptr);
    Assert::IsTrue(!error);

    const JlsRect rect = { 128, 128, 256, 1 };
    vector<uint8_t> decodedData(static_cast<size_t>(rect.Width) * rect.Height);
    decodedData.push_back(0x1f);
    error = JpegLsDecodeRect(decodedData.data(), decodedData.size(), encodedData.data(), encodedData.size(), rect, nullptr, nullptr);
    Assert::IsTrue(!error);

    Assert::IsTrue(memcmp(&decodedBuffer[rect.X + static_cast<size_t>(rect.Y) * 512], decodedData.data(), static_cast<size_t>(rect.Width) * rect.Height) == 0);
    Assert::IsTrue(decodedData[static_cast<size_t>(rect.Width) * rect.Height] == 0x1f);
}


void TestEncodeFromStream(const char* file, int offset, int width, int height, int bpp, int componentCount, interleave_mode ilv, size_t expectedLength)
{
    basic_filebuf<char> myFile; // On the stack
    myFile.open(file, mode_input);
    Assert::IsTrue(myFile.is_open());

    myFile.pubseekoff(static_cast<streamoff>(offset), ios_base::cur);
    const ByteStreamInfo rawStreamInfo = {&myFile, nullptr, 0};

    vector<uint8_t> compressed(static_cast<size_t>(width) * height * componentCount * 2);
    JlsParameters params = JlsParameters();
    params.height = height;
    params.width = width;
    params.components = componentCount;
    params.bitsPerSample = bpp;
    params.interleaveMode = ilv;
    size_t bytesWritten = 0;

    JpegLsEncodeStream(FromByteArray(compressed.data(), static_cast<size_t>(width) * height * componentCount * 2), bytesWritten, rawStreamInfo, params);
    Assert::IsTrue(bytesWritten == expectedLength);

    myFile.close();
}


bool DecodeToPnm(istream& input, ostream& output)
{
    const ByteStreamInfo inputInfo{input.rdbuf(), nullptr, 0};

    auto params = JlsParameters();
    error_code error = JpegLsReadHeaderStream(inputInfo, &params);
    if (error)
        return false;

    input.seekg(0);

    const int maxValue = (1 << params.bitsPerSample) - 1;
    const int bytesPerSample = maxValue > 255 ? 2 : 1;
    vector<uint8_t> outputBuffer(static_cast<size_t>(params.width) * params.height * bytesPerSample * params.components);
    const auto outputInfo = FromByteArray(outputBuffer.data(), outputBuffer.size());
    error = JpegLsDecodeStream(outputInfo, inputInfo, &params);
    if (error)
        return false;

    // PNM format requires most significant byte first (big endian).
    if (bytesPerSample == 2)
    {
        for (auto i = outputBuffer.begin(); i != outputBuffer.end(); i += 2)
        {
            iter_swap(i, i + 1);
        }
    }

    const int magicNumber = params.components == 3 ? 6 : 5;
    output << 'P' << magicNumber << "\n" << params.width << ' ' << params.height << "\n" << maxValue << "\n";
    output.write(reinterpret_cast<char*>(outputBuffer.data()), outputBuffer.size());

    return true;
}


vector<int> readPnmHeader(istream& pnmFile)
{
    vector<int> readValues;

    const auto first = static_cast<char>(pnmFile.get());

    // All portable anymap format (PNM) start with the character P.
    if (first != 'P')
        return readValues;

    while (readValues.size() < 4)
    {
        string bytes;
        getline(pnmFile, bytes);
        stringstream line(bytes);

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
bool EncodePnm(istream& pnmFile, const ostream& jlsFileStream)
{
    vector<int> readValues = readPnmHeader(pnmFile);
    if (readValues.size() !=4)
        return false;

    JlsParameters params{};
    params.components = readValues[0] == 6 ? 3 : 1;
    params.width = readValues[1];
    params.height = readValues[2];
    params.bitsPerSample = log_2(readValues[3] + 1);
    params.interleaveMode = params.components == 3 ? interleave_mode::line : interleave_mode::none;

    const int bytesPerSample = (params.bitsPerSample + 7) / 8;
    vector<uint8_t> inputBuffer(static_cast<size_t>(params.width) * params.height * bytesPerSample * params.components);
    pnmFile.read(reinterpret_cast<char*>(inputBuffer.data()), inputBuffer.size());
    if (!pnmFile.good())
        return false;

    // PNM format is stored with most significant byte first (big endian).
    if (bytesPerSample == 2)
    {
        for (auto i = inputBuffer.begin(); i != inputBuffer.end(); i += 2)
        {
            iter_swap(i, i + 1);
        }
    }

    const auto rawStreamInfo = FromByteArray(inputBuffer.data(), inputBuffer.size());
    const ByteStreamInfo jlsStreamInfo = {jlsFileStream.rdbuf(), nullptr, 0};

    size_t bytesWritten = 0;
    JpegLsEncodeStream(jlsStreamInfo, bytesWritten, rawStreamInfo, params);
    return true;
}


bool ComparePnm(istream& pnmFile1, istream& pnmFile2)
{
    vector<int> header1 = readPnmHeader(pnmFile1);
    if (header1.size() != 4)
    {
        cout << "Cannot read header from input file 1\n";
        return false;
    }

    vector<int> header2 = readPnmHeader(pnmFile2);
    if (header2.size() != 4)
    {
        cout << "Cannot read header from input file 2\n";
        return false;
    }

    if (header1[0] != header2[0])
    {
        cout << "Header type " << header1[0] << " is not equal with type "<< header2[0] << "\n";
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
    const auto bytesPerSample = header1[3] > 255 ? 2 : 1;

    const size_t byteCount = width * height * bytesPerSample;
    vector<uint8_t> bytes1(byteCount);
    vector<uint8_t> bytes2(byteCount);

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


////void TestDecodeFromStream(const char* strNameEncoded)
////{
////    basic_filebuf<char> jlsFile;
////    jlsFile.open(strNameEncoded, mode_input);
////    Assert::IsTrue(jlsFile.is_open());
////    ByteStreamInfo compressedByteStream = {&jlsFile, nullptr, 0};
////
////    auto params = JlsParameters();
////    auto err = JpegLsReadHeaderStream(compressedByteStream, &params, nullptr);
////    Assert::IsTrue(err == jpegls_errc::OK);
////
////    jlsFile.pubseekpos(ios::beg, ios_base::in);
////
////    basic_stringbuf<char> buf;
////    ByteStreamInfo rawStreamInfo = {&buf, nullptr, 0};
////
////    err = JpegLsDecodeStream(rawStreamInfo, compressedByteStream, nullptr, nullptr);
////    ////size_t outputCount = buf.str().size();
////
////    Assert::IsTrue(err == jpegls_errc::OK);
////    //Assert::IsTrue(outputCount == 512 * 512);
////}


jpegls_errc DecodeRaw(const char* strNameEncoded, const char* strNameOutput)
{
    fstream jlsFile(strNameEncoded, mode_input);
    const ByteStreamInfo compressedByteStream{jlsFile.rdbuf(), nullptr, 0};

    fstream rawFile(strNameOutput, mode_output);
    const ByteStreamInfo rawStream{rawFile.rdbuf(), nullptr, 0};

    const auto value = JpegLsDecodeStream(rawStream, compressedByteStream, nullptr);
    jlsFile.close();
    rawFile.close();

    return value;
}


void TestEncodeFromStream()
{
    ////TestDecodeFromStream("test/user_supplied/output.jls");

    TestEncodeFromStream("test/0015.raw", 0, 1024, 1024, 8, 1, interleave_mode::none, 0x3D3ee);
    ////TestEncodeFromStream("test/MR2_UNC", 1728, 1024, 1024, 16, 1,0, 0x926e1);
    TestEncodeFromStream("test/conformance/TEST8.PPM", 15, 256, 256, 8, 3, interleave_mode::sample, 99734);
    TestEncodeFromStream("test/conformance/TEST8.PPM", 15, 256, 256, 8, 3, interleave_mode::line, 100615);
}


void UnitTest()
{
    try
    {
        //// TestBadImage();

        cout << "Test Conformance\n";
        TestEncodeFromStream();
        TestConformance();

        TestDecodeRect();

        cout << "Test Traits\n";
        TestTraits16bit();
        TestTraits8bit();

        cout << "Windows bitmap BGR/BGRA output\n";
        TestBgr();
        TestBgra();

        cout << "Test Small buffer\n";
        TestTooSmallOutputBuffer();

        TestFailOnTooSmallOutputBuffer();

        cout << "Test Color transform equivalence on HP images\n";
        TestColorTransforms_HpImages();

        cout << "Test Annex H3\n";
        TestSampleAnnexH3();

        TestNoiseImage();
        TestNoiseImageWithCustomReset();

        cout << "Test robustness\n";
        TestDecodeBitStreamWithNoMarkerStart();
        TestDecodeBitStreamWithUnsupportedEncoding();
        TestDecodeBitStreamWithUnknownJpegMarker();
    }
    catch (const UnitTestException&)
    {
        cout << "==> Unit test failed <==\n";
    }
}

} // namespace


int main(const int argc, const char * const argv[])
{
    if (argc == 1)
    {
        cout << "CharLS test runner.\nOptions: -unittest, -bitstreamdamage, -performance[:loop-count], -decodeperformance[:loop-count], -decoderaw -encodepnm -decodetopnm -comparepnm\n";
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; ++i)
    {
        string str = argv[i];
        if (str == "-unittest")
        {
            UnitTest();
            continue;
        }

        if (str == "-decoderaw")
        {
            if (i != 1 || argc != 4)
            {
                cout << "Syntax: -decoderaw inputfile outputfile\n";
                return EXIT_FAILURE;
            }
            return make_error_code(DecodeRaw(argv[2], argv[3])) ? EXIT_FAILURE : EXIT_SUCCESS;
        }

        if (str == "-decodetopnm")
        {
            if (i != 1 || argc != 4)
            {
                cout << "Syntax: -decodetopnm inputfile outputfile\n";
                return EXIT_FAILURE;
            }
            fstream pnmFile(argv[3], mode_output);
            fstream jlsFile(argv[2], mode_input);

            return DecodeToPnm(jlsFile, pnmFile) ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        if (str == "-encodepnm")
        {
            if (i != 1 || argc != 4)
            {
                cout << "Syntax: -encodepnm inputfile outputfile\n";
                return EXIT_FAILURE;
            }
            fstream pnmFile(argv[2], mode_input);
            const fstream jlsFile(argv[3], mode_output);

            return EncodePnm(pnmFile, jlsFile) ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        if (str == "-comparepnm")
        {
            if (i != 1 || argc != 4)
            {
                cout << "Syntax: -encodepnm inputfile outputfile\n";
                return EXIT_FAILURE;
            }
            fstream pnmFile1(argv[2], mode_input);
            fstream pnmFile2(argv[3], mode_input);

            return ComparePnm(pnmFile1, pnmFile2) ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        if (str == "-bitstreamdamage")
        {
            DamagedBitStreamTests();
            continue;
        }

        if (str.compare(0, 12, "-performance") == 0)
        {
            int loopCount = 1;

            // Extract the optional loop count from the command line. Longer running tests make the measurements more reliable.
            auto index = str.find(':');
            if (index != string::npos)
            {
                loopCount = stoi(str.substr(++index));
                if (loopCount < 1)
                {
                    cout << "Loop count not understood or invalid: %s" << str << "\n";
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
            if (index != string::npos)
            {
                loopCount = stoi(str.substr(++index));
                if (loopCount < 1)
                {
                    cout << "Loop count not understood or invalid: " << str << "\n";
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

        cout << "Option not understood: " << argv[i] << "\n";
        break;
    }

    return EXIT_SUCCESS;
}
