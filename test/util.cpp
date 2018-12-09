// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include "util.h"
#include "portable_anymap_file.h"
#include "gettime.h"

#include <iostream>
#include <vector>
#include <iomanip>

using std::cout;
using std::cerr;
using std::setw;
using std::setprecision;
using std::vector;
using std::error_code;
using std::swap;
using charls::InterleaveMode;
using charls::ColorTransformation;


namespace
{

MSVC_WARNING_SUPPRESS(26497) // cannot be marked constexpr, check must be executed at runtime.

bool IsMachineLittleEndian() noexcept
{
    const int a = 0xFF000001;
    const auto* chars = reinterpret_cast<const char*>(&a);
    return chars[0] == 0x01;
}

MSVC_WARNING_UNSUPPRESS()


} // namespace


void FixEndian(vector<uint8_t>* rgbyte, bool littleEndianData)
{
    if (littleEndianData == IsMachineLittleEndian())
        return;

    for (size_t i = 0; i < rgbyte->size()-1; i += 2)
    {
        swap((*rgbyte)[i], (*rgbyte)[i + 1]);
    }
}


vector<uint8_t> ReadFile(const char* filename, long offset, size_t bytes)
{
    FILE* pfile = fopen(filename, "rb");
    if (!pfile)
    {
        cerr << "Could not open %s\n" << filename << "\n";
        throw UnitTestException();
    }

    fseek(pfile, 0, SEEK_END);
    const auto cbyteFile = static_cast<int>(ftell(pfile));
    fseek(pfile, offset, SEEK_SET);

    if (offset < 0)
    {
        Assert::IsTrue(bytes != 0);
        offset = static_cast<long>(cbyteFile - bytes);
    }
    if (bytes == 0)
    {
        bytes = static_cast<size_t>(cbyteFile) - offset;
    }

    vector<uint8_t> buffer(bytes);
    const size_t bytesRead = fread(buffer.data(), 1, buffer.size(), pfile);
    fclose(pfile);
    if (bytesRead != buffer.size())
        throw UnitTestException();

    return buffer;
}


void WriteFile(const char* filename, vector<uint8_t>& buffer)
{
    FILE* pfile = fopen(filename, "wb");
    if( !pfile )
    {
        cerr << "Could not open " << filename << "\n";
        return;
    }

    fwrite(&buffer[0],1, buffer.size(), pfile);
    fclose(pfile);
}


void TestRoundTrip(const char* strName, const vector<uint8_t>& rgbyteRaw, Size size, int cbit, int ccomp, int loopCount)
{
    JlsParameters params = JlsParameters();
    params.components = ccomp;
    params.bitsPerSample = cbit;
    params.height = static_cast<int>(size.cy);
    params.width = static_cast<int>(size.cx);

    TestRoundTrip(strName, rgbyteRaw, params, loopCount);
}


void TestRoundTrip(const char* strName, const vector<uint8_t>& rgbyteRaw, JlsParameters& params, int loopCount)
{
    vector<uint8_t> rgbyteCompressed(params.height * params.width * params.components * params.bitsPerSample / 4);

    vector<uint8_t> rgbyteOut(static_cast<size_t>(params.height) * params.width * ((params.bitsPerSample + 7) / 8) * params.components);

    if (params.components == 4)
    {
        params.interleaveMode = InterleaveMode::Line;
    }
    else if (params.components == 3)
    {
        params.interleaveMode = InterleaveMode::Line;
        params.colorTransformation = ColorTransformation::HP1;
    }

    size_t compressedLength = 0;
    const double dwtimeEncodeStart = getTime();
    for (int i = 0; i < loopCount; ++i)
    {
        const error_code error = JpegLsEncode(&rgbyteCompressed[0], rgbyteCompressed.size(), &compressedLength,
            &rgbyteRaw[0], rgbyteOut.size(), &params, nullptr);
        Assert::IsTrue(!error);
    }
    const double dwtimeEncodeComplete = getTime();

    const double dwtimeDecodeStart = getTime();
    for (int i = 0; i < loopCount; ++i)
    {
        const error_code error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], compressedLength, nullptr, nullptr);
        Assert::IsTrue(!error);
    }
    const double dwtimeDecodeComplete = getTime();

    const double bitspersample = 1.0 * compressedLength * 8 / (static_cast<double>(params.components) * params.height * params.width);
    cout << "RoundTrip test for: " << strName << "\n\r";
    const double encodeTime = (dwtimeEncodeComplete - dwtimeEncodeStart) / loopCount;
    const double decodeTime = (dwtimeDecodeComplete - dwtimeDecodeStart) / loopCount;
    const double symbolRate = (static_cast<double>(params.components) * params.height * params.width) / (1000.0 * decodeTime);

    cout << "Size:" << setw(10) << params.width << "x" << params.height << setw(7) << setprecision(2) <<
        ", Encode time:" << encodeTime << " ms, Decode time:" << decodeTime <<
        " ms, Bits per sample:" << bitspersample << ", Decode rate:" << symbolRate << " M/s\n";

    const uint8_t* pbyteOut = rgbyteOut.data();
    for (size_t i = 0; i < rgbyteOut.size(); ++i)
    {
        if (rgbyteRaw[i] != pbyteOut[i])
        {
            Assert::IsTrue(false);
            break;
        }
    }
}


void TestFile(const char* filename, int ioffs, Size size2, int cbit, int ccomp, bool littleEndianFile, int loopCount)
{
    const size_t byteCount = size2.cx * size2.cy * ccomp * ((cbit + 7)/8);
    vector<uint8_t> uncompressedBuffer = ReadFile(filename, ioffs, byteCount);

    if (cbit > 8)
    {
        FixEndian(&uncompressedBuffer, littleEndianFile);
    }

    TestRoundTrip(filename, uncompressedBuffer, size2, cbit, ccomp, loopCount);
}


void test_portable_anymap_file(const char* filename, int loopCount)
{
    portable_anymap_file anymapFile(filename);

    TestRoundTrip(filename, anymapFile.image_data(), Size(anymapFile.width(), anymapFile.height()),
        anymapFile.bits_per_sample(), anymapFile.component_count(), loopCount);
}
