// 
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#include "config.h"
#include "util.h"
#include "../src/charls.h"

#include "../src/defaulttraits.h"
#include "../src/losslesstraits.h"
#include "../src/processline.h"

#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace charls;


typedef const char* SZC;

const std::ios_base::openmode mode_input  = std::ios_base::in  | std::ios::binary;
const std::ios_base::openmode mode_output = std::ios_base::out | std::ios::binary;


bool ScanFile(SZC strNameEncoded, std::vector<BYTE>* rgbyteFile, JlsParameters* params)
{
    if (!ReadFile(strNameEncoded, rgbyteFile))
    {
        Assert::IsTrue(false);
        return false;
    }
    std::basic_filebuf<char> jlsFile; 
    jlsFile.open(strNameEncoded, mode_input);

    ByteStreamInfo rawStreamInfo = {&jlsFile, nullptr, 0};

    auto err = JpegLsReadHeaderStream(rawStreamInfo, params, nullptr);
    Assert::IsTrue(err == ApiResult::OK);
    return err == ApiResult::OK;
}


void TestTraits16bit()
{
    auto traits1 = DefaultTraitsT<uint16_t, uint16_t>(4095,0);
    auto traits2 = LosslessTraitsT<uint16_t, 12>();

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
    auto traits1 = DefaultTraitsT<uint8_t, uint8_t>(255,0);
    auto traits2 = LosslessTraitsT<uint8_t, 8>();

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


std::vector<BYTE> MakeSomeNoise(int length, int bitcount, int seed)
{
    srand(seed);
    std::vector<BYTE> rgbyteNoise(length);
    BYTE mask = (1 << bitcount) - 1;
    for (int icol= 0; icol<length; ++icol)
    {
        BYTE val = BYTE(rand());
        rgbyteNoise[icol] = BYTE(val & mask);
    }
    return rgbyteNoise;
}


void TestNoiseImage()
{
    Size size2 = Size(512, 512);

    for (int bitDepth = 8; bitDepth >=2; --bitDepth)
    {
        std::stringstream label;
        label << "noise, bitdepth: " << bitDepth;

        std::vector<BYTE> noiseBytes = MakeSomeNoise(size2.cx * size2.cy, bitDepth, 21344);
        TestRoundTrip(label.str().c_str(), noiseBytes, size2, bitDepth, 1);
    }
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
    std::vector<BYTE> outputBuffer1(1);
    auto result = JpegLsEncode(outputBuffer1.data(), outputBuffer1.size(), &compressedLength, inputBuffer.data(), inputBuffer.size(), &params, nullptr);
    Assert::IsTrue(result == ApiResult::CompressedBufferTooSmall);

    // Trigger a "buffer too small"" when writing the encoded pixel bytes.
    std::vector<BYTE> outputBuffer2(100);
    result = JpegLsEncode(outputBuffer2.data(), outputBuffer2.size(), &compressedLength, inputBuffer.data(), inputBuffer.size(), &params, nullptr);
    Assert::IsTrue(result == ApiResult::CompressedBufferTooSmall);
}


void TestBgra()
{
    char rgbyteTest[] = "RGBARGBARGBARGBA1234";
    char rgbyteComp[] = "BGRABGRABGRABGRA1234";
    TransformRgbToBgr(rgbyteTest, 4, 4);
    Assert::IsTrue(strcmp(rgbyteTest, rgbyteComp) == 0);
}


void TestBgr()
{
    JlsParameters params;
    std::vector<BYTE> rgbyteEncoded;
    ScanFile("test/conformance/T8C2E3.JLS", &rgbyteEncoded, &params);
    std::vector<BYTE> rgbyteDecoded(params.width * params.height * params.components);    

    params.outputBgr = true;

    auto err = JpegLsDecode(&rgbyteDecoded[0], rgbyteDecoded.size(), &rgbyteEncoded[0], rgbyteEncoded.size(), &params, nullptr);
    Assert::IsTrue(err == ApiResult::OK);

    Assert::IsTrue(rgbyteDecoded[0] == 0x69);
    Assert::IsTrue(rgbyteDecoded[1] == 0x77);
    Assert::IsTrue(rgbyteDecoded[2] == 0xa1);
    Assert::IsTrue(rgbyteDecoded[params.width * 6 + 3] == 0x2d);
    Assert::IsTrue(rgbyteDecoded[params.width * 6 + 4] == 0x43);
    Assert::IsTrue(rgbyteDecoded[params.width * 6 + 5] == 0x4d);  
}


void TestTooSmallOutputBuffer()
{
    std::vector<BYTE> rgbyteCompressed;
    if (!ReadFile("test/lena8b.jls", &rgbyteCompressed, 0))
        return;

    std::vector<BYTE> rgbyteOut(512 * 511);
    auto error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], rgbyteCompressed.size(), nullptr, nullptr);

    Assert::IsTrue(error == ApiResult::UncompressedBufferTooSmall);
}


void TestBadImage()
{
    std::vector<BYTE> rgbyteCompressed;
    if (!ReadFile("test/BadCompressedStream.jls", &rgbyteCompressed, 0))
        return;

    std::vector<BYTE> rgbyteOut(2500 * 3000 * 2);
    auto error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], rgbyteCompressed.size(), nullptr, nullptr);

    Assert::IsTrue(error == ApiResult::UncompressedBufferTooSmall);
}


void TestDecodeBitStreamWithNoMarkerStart()
{
    BYTE encodedData[2] = { 0x33, 0x33 };
    BYTE output[1000];

    auto error = JpegLsDecode(output, 1000, encodedData, 2, nullptr, nullptr);
    Assert::IsTrue(error == ApiResult::MissingJpegMarkerStart);
}


void TestDecodeBitStreamWithUnsupportedEncoding()
{
    BYTE encodedData[6] = { 0xFF, 0xD8, // Start Of Image (JPEG_SOI)
                            0xFF, 0xC3, // Start Of Frame (lossless, huffman) (JPEG_SOF_3)
                            0x00, 0x00  // Lenght of data of the marker
                          };
    BYTE output[1000];

    auto error = JpegLsDecode(output, 1000, encodedData, 6, nullptr, nullptr);
    Assert::IsTrue(error == ApiResult::UnsupportedEncoding);
}


void TestDecodeBitStreamWithUnknownJpegMarker()
{
    BYTE encodedData[6] = { 0xFF, 0xD8, // Start Of Image (JPEG_SOI)
        0xFF, 0x01, // Undefined marker
        0x00, 0x00  // Lenght of data of the marker
    };
    BYTE output[1000];

    auto error = JpegLsDecode(output, 1000, encodedData, 6, nullptr, nullptr);
    Assert::IsTrue(error == ApiResult::UnknownJpegMarker);
}


void TestDecodeRect()
{
    std::vector<BYTE> rgbyteCompressed;
    JlsParameters params;
    if (!ScanFile("test/lena8b.jls", &rgbyteCompressed, &params))
        return;

    std::vector<BYTE> rgbyteOutFull(params.width*params.height*params.components);
    auto error = JpegLsDecode(&rgbyteOutFull[0], rgbyteOutFull.size(), &rgbyteCompressed[0], rgbyteCompressed.size(), nullptr, nullptr);
    Assert::IsTrue(error == ApiResult::OK);

    JlsRect rect = { 128, 128, 256, 1 };
    std::vector<BYTE> rgbyteOut(rect.Width * rect.Height);
    rgbyteOut.push_back(0x1f);
    error = JpegLsDecodeRect(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], rgbyteCompressed.size(), rect, nullptr, nullptr);
    Assert::IsTrue(error == ApiResult::OK);

    Assert::IsTrue(memcmp(&rgbyteOutFull[rect.X + rect.Y*512], &rgbyteOut[0], rect.Width * rect.Height) == 0);
    Assert::IsTrue(rgbyteOut[rect.Width * rect.Height] == 0x1f);
}


void TestEncodeFromStream(const char* file, int offset, int width, int height, int bpp, int ccomponent, InterleaveMode ilv, size_t expectedLength)
{
    std::basic_filebuf<char> myFile; // On the stack
    myFile.open(file, mode_input);
    Assert::IsTrue(myFile.is_open());

    myFile.pubseekoff(std::streamoff(offset), std::ios_base::cur);
    ByteStreamInfo rawStreamInfo = {&myFile, nullptr, 0};

    BYTE* compressed = new BYTE[width * height * ccomponent * 2];
    JlsParameters params = JlsParameters();
    params.height = height;
    params.width = width;
    params.components = ccomponent;
    params.bitsPerSample = bpp;
    params.interleaveMode = ilv;
    size_t bytesWritten = 0;

    JpegLsEncodeStream(FromByteArray(compressed, width * height * ccomponent * 2), bytesWritten, rawStreamInfo, params, nullptr);
    Assert::IsTrue(bytesWritten == expectedLength);

    delete[] compressed;
    myFile.close();
}


bool DecodeToPnm(std::istream& input, std::ostream& output)
{
    ByteStreamInfo inputInfo = { input.rdbuf(), nullptr, 0 };

    auto params = JlsParameters();
    auto result = JpegLsReadHeaderStream(inputInfo, &params, nullptr);
    if (result != ApiResult::OK)
        return false;
    input.seekg(0);

    int maxValue = (1 << params.bitsPerSample) - 1;
    int bytesPerSample = maxValue > 255 ? 2 : 1;
    std::vector<uint8_t> outputBuffer(params.width * params.height * bytesPerSample);
    auto outputInfo = FromByteArray(outputBuffer.data(), outputBuffer.size());
    JpegLsDecodeStream(outputInfo, inputInfo, &params, nullptr);

    // PNM format requires most significant byte first (big endian).
    if (bytesPerSample == 2)
    {
        for (auto i = outputBuffer.begin(); i != outputBuffer.end(); i += 2)
        {
            std::iter_swap(i, i + 1);
        }
    }

    int magicNumber = params.components == 3 ? 6 : 5;
    output << 'P' << magicNumber << std::endl << params.width << ' ' << params.height << std::endl << maxValue << std::endl;
    output.write(reinterpret_cast<char*>(outputBuffer.data()), outputBuffer.size());

    return true;
}


std::vector<int> readPnmHeader(std::istream& pnmFile)
{
    std::vector<int> readValues;
    
    char first = (char) pnmFile.get();

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


bool EncodePnm(std::istream& pnmFile, std::ostream& jlsFileStream)
{
    std::vector<int> readValues = readPnmHeader(pnmFile);
    if (readValues.size() !=4)
        return false;

    ByteStreamInfo rawStreamInfo = {pnmFile.rdbuf(), nullptr, 0};
    ByteStreamInfo jlsStreamInfo = {jlsFileStream.rdbuf(), nullptr, 0};

    JlsParameters params = JlsParameters();
    int componentCount = readValues[0] == 6 ? 3 : 1;
    params.width = readValues[1];
    params.height = readValues[2];
    params.components = componentCount;
    params.bitsPerSample = log_2(readValues[3]+1);
    params.interleaveMode = componentCount == 3 ? InterleaveMode::Line : InterleaveMode::None;
    params.colorTransformation = ColorTransformation::BigEndian;
    size_t bytesWritten = 0;

    JpegLsEncodeStream(jlsStreamInfo, bytesWritten, rawStreamInfo, params, nullptr);
    return true;
}


void ComparePnm(std::istream& pnmFile1, std::istream& pnmFile2)
{
    std::vector<int> header1 = readPnmHeader(pnmFile1);
    if (header1.size() != 4)
    {
        printf("Cannot read header from input file 1\r\n");
        return;
    }

    std::vector<int> header2 = readPnmHeader(pnmFile2);
    if (header2.size() != 4)
    {
        printf("Cannot read header from input file 2\r\n");
        return;
    }

    if (header1[0] != header2[0])
    {
        printf("Header type %i is not equal with type %i\r\n", header1[0], header2[0]);
        return;
    }

    auto width = header1[1];
    if (width != header2[1])
    {
        printf("Width %i is not equal with width %i\r\n", width, header2[1]);
        return;
    }

    auto height = header1[2];
    if (height != header2[2])
    {
        printf("Height %i is not equal with height %i\r\n", height, header2[2]);
        return;
    }

    if (header1[3] != header2[3])
    {
        printf("max-value %i is not equal with max-value %i\r\n", header1[3], header2[3]);
        return;
    }
    auto bytesPerSample = header1[3] > 255 ? 2 : 1;

    size_t byteCount = width * height * bytesPerSample;
    std::vector<uint8_t> bytes1(byteCount);
    std::vector<uint8_t> bytes2(byteCount);

    pnmFile1.read(reinterpret_cast<char*>(&bytes1[0]), byteCount);
    pnmFile2.read(reinterpret_cast<char*>(&bytes2[0]), byteCount);

    for (auto x = 0; x < height; ++x)
    {
        for (auto y = 0; y < width; y += bytesPerSample)
        {
            if (bytesPerSample == 1)
            {
                if (bytes1[(x * width) + y] != bytes2[(x * width) + y])
                {
                    printf("Values of the 2 files are different, height:%i, width:%i\r\n", x, y);
                    return;
                }
            }
            else
            {
                if (bytes1[(x * width) + y] != bytes2[(x * width) + y] ||
                    bytes1[(x * width) + (y + 1)] != bytes2[(x * width) + (y + 1)])
                {
                    printf("Values of the 2 files are different, height:%i, width:%i\r\n", x, y);
                    return;
                }
            }
        }
    }

    printf("Values of the 2 files are equal\r\n");
}


void TestDecodeFromStream(const char* strNameEncoded)
{
    std::basic_filebuf<char> jlsFile; 
    jlsFile.open(strNameEncoded, mode_input);
    Assert::IsTrue(jlsFile.is_open());
    ByteStreamInfo compressedByteStream = {&jlsFile, nullptr, 0};

    auto params = JlsParameters();
    auto err = JpegLsReadHeaderStream(compressedByteStream, &params, nullptr);
    Assert::IsTrue(err == ApiResult::OK);

    jlsFile.pubseekpos(std::ios::beg, std::ios_base::in);

    std::basic_stringbuf<char> buf;
    ByteStreamInfo rawStreamInfo = {&buf, nullptr, 0};

    err = JpegLsDecodeStream(rawStreamInfo, compressedByteStream, nullptr, nullptr);
    ////size_t outputCount = buf.str().size();

    Assert::IsTrue(err == ApiResult::OK);
    //Assert::IsTrue(outputCount == 512 * 512);
}


ApiResult DecodeRaw(const char* strNameEncoded, const char* strNameOutput)
{
    std::fstream jlsFile(strNameEncoded, mode_input);
    ByteStreamInfo compressedByteStream = {jlsFile.rdbuf(), nullptr, 0};

    std::fstream rawFile(strNameOutput, mode_output); 
    ByteStreamInfo rawStream = {rawFile.rdbuf(), nullptr, 0};

    auto value = JpegLsDecodeStream(rawStream, compressedByteStream, nullptr, nullptr);
    jlsFile.close();
    rawFile.close();

    return value;
}


void TestEncodeFromStream()
{
    ////TestDecodeFromStream("test/user_supplied/output.jls");

    TestEncodeFromStream("test/0015.raw", 0, 1024, 1024, 8, 1, InterleaveMode::None, 0x3D3ee);
    //TestEncodeFromStream("test/MR2_UNC", 1728, 1024, 1024, 16, 1,0, 0x926e1);
    TestEncodeFromStream("test/conformance/TEST8.PPM", 15, 256, 256, 8, 3, InterleaveMode::Sample, 99734);
    TestEncodeFromStream("test/conformance/TEST8.PPM", 15, 256, 256, 8, 3, InterleaveMode::Line, 100615);
}


void TestColorTransforms_HpImages();
void TestConformance();
void TestSampleAnnexH3();
void PerformanceTests(int loopCount);
void DecodePerformanceTests(int loopCount);
void DamagedBitstreamTests();
void TestDicomWG4Images();

void UnitTest()
{
    try
    {
        //  TestBadImage();

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


int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        printf("CharLS test runner.\r\nOptions: -unittest, -bitstreamdamage, -performance[:loop count], -dontwait -decoderaw -encodepnm -decodetopnm -comparepnm\r\n");
        return 0;
    }

    bool wait = true;
    for (int i = 1; i < argc; ++i)
    {
        std::string str = argv[i];
        if (str.compare("-unittest") == 0)
        {
            UnitTest();
            continue;
        }

        if (str.compare("-decoderaw") == 0)
        {
            if (i != 1 || argc != 4)
            {
                printf("Syntax: -decoderaw inputfile outputfile \r\n");
                return 0;
            }
            return static_cast<int>(DecodeRaw(argv[2], argv[3]));
        }

        if (str.compare("-decodetopnm") == 0)
        {
            if (i != 1 || argc != 4)
            {
                printf("Syntax: -decodetopnm inputfile outputfile \r\n");
                return 0;
            }
            std::fstream pnmFile(argv[3], mode_output); 
            std::fstream jlsFile(argv[2], mode_input);

            return DecodeToPnm(jlsFile, pnmFile);
        }

        if (str.compare("-encodepnm") == 0)
        {
            if (i != 1 || argc != 4)
            {
                printf("Syntax: -encodepnm inputfile outputfile\r\n");
                return 0;
            }
            std::fstream pnmFile(argv[2], mode_input); 
            std::fstream jlsFile(argv[3], mode_output); 
    
            return EncodePnm(pnmFile,jlsFile);
        }

        if (str.compare("-comparepnm") == 0)
        {
            if (i != 1 || argc != 4)
            {
                printf("Syntax: -encodepnm inputfile outputfile\r\n");
                return 0;
            }
            std::fstream pnmFile1(argv[2], mode_input);
            std::fstream pnmFile2(argv[3], mode_input);

            ComparePnm(pnmFile1, pnmFile2);
            return 0;
        }

        if (str.compare("-bitstreamdamage") == 0)
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

        if (str.compare("-dicom") == 0)
        {
            TestDicomWG4Images();
            continue;
        }

        if (str.compare("-dontwait") == 0)
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
        return 0;
    }
}
