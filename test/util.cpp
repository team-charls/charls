//
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use.
//


#include "util.h"
#include "portable_anymap_file.h"
#include "gettime.h"
#include <iostream>
#include <vector>


using namespace charls;

namespace
{

bool IsMachineLittleEndian() noexcept
{
    const int a = 0xFF000001;
    const char* chars = reinterpret_cast<const char*>(&a);
    return chars[0] == 0x01;
}

} // namespace


void FixEndian(std::vector<uint8_t>* rgbyte, bool littleEndianData)
{
    if (littleEndianData == IsMachineLittleEndian())
        return;

    for (size_t i = 0; i < rgbyte->size()-1; i += 2)
    {
        std::swap((*rgbyte)[i], (*rgbyte)[i + 1]);
    }
}


bool ReadFile(const char* filename, std::vector<uint8_t>* pvec, long offset, size_t bytes)
{
    FILE* pfile = fopen(filename, "rb");
    if (!pfile)
    {
        fprintf(stderr, "Could not open %s\n", filename);
        return false;
    }

    fseek(pfile, 0, SEEK_END);
    const auto cbyteFile = static_cast<int>(ftell(pfile));
    if (offset < 0)
    {
        Assert::IsTrue(bytes != 0);
        offset = static_cast<long>(cbyteFile - bytes);
    }
    if (bytes == 0)
    {
        bytes = static_cast<size_t>(cbyteFile) - offset;
    }

    fseek(pfile, offset, SEEK_SET);
    pvec->resize(bytes);
    const size_t bytesRead = fread(&(*pvec)[0],1, pvec->size(), pfile);
    fclose(pfile);
    return bytesRead == pvec->size();
}


void WriteFile(const char* filename, std::vector<uint8_t>& buffer)
{
    FILE* pfile = fopen(filename, "wb");
    if( !pfile )
    {
        fprintf( stderr, "Could not open %s\n", filename );
        return;
    }

    fwrite(&buffer[0],1, buffer.size(), pfile);
    fclose(pfile);
}


void TestRoundTrip(const char* strName, const std::vector<uint8_t>& rgbyteRaw, Size size, int cbit, int ccomp, int loopCount)
{
    JlsParameters params = JlsParameters();
    params.components = ccomp;
    params.bitsPerSample = cbit;
    params.height = static_cast<int>(size.cy);
    params.width = static_cast<int>(size.cx);

    TestRoundTrip(strName, rgbyteRaw, params, loopCount);
}


void TestRoundTrip(const char* strName, const std::vector<uint8_t>& rgbyteRaw, JlsParameters& params, int loopCount)
{
    std::vector<uint8_t> rgbyteCompressed(params.height * params.width * params.components * params.bitsPerSample / 4);

    std::vector<uint8_t> rgbyteOut(static_cast<size_t>(params.height) * params.width * ((params.bitsPerSample + 7) / 8) * params.components);

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
        const auto err = JpegLsEncode(&rgbyteCompressed[0], rgbyteCompressed.size(), &compressedLength, &rgbyteRaw[0], rgbyteOut.size(), &params, nullptr);
        Assert::IsTrue(err == ApiResult::OK);
    }
    const double dwtimeEncodeComplete = getTime();

    const double dwtimeDecodeStart = getTime();
    for (int i = 0; i < loopCount; ++i)
    {
        const auto err = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], compressedLength, nullptr, nullptr);
        Assert::IsTrue(err == ApiResult::OK);
    }
    const double dwtimeDecodeComplete = getTime();

    const double bitspersample = 1.0 * compressedLength * 8 / (static_cast<double>(params.components) * params.height * params.width);
    std::cout << "RoundTrip test for: " << strName << "\n\r";
    const double encodeTime = (dwtimeEncodeComplete - dwtimeEncodeStart) / loopCount;
    const double decodeTime = (dwtimeDecodeComplete - dwtimeDecodeStart) / loopCount;
    const double symbolRate = (static_cast<double>(params.components) * params.height * params.width) / (1000.0 * decodeTime);
    printf("Size:%4dx%4d, Encode time:%7.2f ms, Decode time:%7.2f ms, Bits per sample:%5.2f, Decode rate:%5.1f M/s \n\r", params.width, params.height, encodeTime, decodeTime, bitspersample, symbolRate);
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
    std::vector<uint8_t> rgbyteUncompressed;

    if (!ReadFile(filename, &rgbyteUncompressed, ioffs, byteCount))
        return;

    if (cbit > 8)
    {
        FixEndian(&rgbyteUncompressed, littleEndianFile);
    }

    TestRoundTrip(filename, rgbyteUncompressed, size2, cbit, ccomp, loopCount);
}


void test_portable_anymap_file(const char* filename, int loopCount)
{
    portable_anymap_file anymapFile(filename);

    TestRoundTrip(filename, anymapFile.image_data(), Size(anymapFile.width(), anymapFile.height()),
        anymapFile.bits_per_sample(), anymapFile.component_count(), loopCount);
}
