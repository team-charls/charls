//
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use.
//


#include "config.h"
#include "util.h"
#include "time.h"

#include "../src/charls.h"

#include <iostream>
#include <vector>


using namespace charls;

namespace
{

bool IsMachineLittleEndian()
{
    int a = 0xFF000001;
    char* chars = reinterpret_cast<char*>(&a);
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


bool ReadFile(const char* filename, std::vector<uint8_t>* pvec, int offset, int bytes)
{
    FILE* pfile = fopen(filename, "rb");
    if (!pfile)
    {
        fprintf(stderr, "Could not open %s\n", filename);
        return false;
    }

    fseek(pfile, 0, SEEK_END);
    int cbyteFile = static_cast<int>(ftell(pfile));
    if (offset < 0)
    {
        Assert::IsTrue(bytes != 0);
        offset = cbyteFile - bytes;
    }
    if (bytes == 0)
    {
        bytes = cbyteFile - offset;
    }

    fseek(pfile, offset, SEEK_SET);
    pvec->resize(bytes);
    size_t bytesRead = fread(&(*pvec)[0],1, pvec->size(), pfile);
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

void TestRoundTrip(const char* strName, std::vector<uint8_t>& rgbyteRaw, Size size, int cbit, int ccomp, int loopCount)
{
    JlsParameters params = JlsParameters();
    params.components = ccomp;
    params.bitsPerSample = cbit;
    params.height = size.cy;
    params.width = size.cx;

    TestRoundTrip(strName, rgbyteRaw, params, loopCount);
}


void TestRoundTrip(const char* strName, std::vector<uint8_t>& rgbyteRaw, JlsParameters& params, int loopCount)
{
    std::vector<uint8_t> rgbyteCompressed(params.height * params.width * params.components * params.bitsPerSample / 4);

    std::vector<uint8_t> rgbyteOut(params.height * params.width * ((params.bitsPerSample + 7) / 8) * params.components);

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
    double dwtimeEncodeStart = getTime();
    for (int i = 0; i < loopCount; ++i)
    {
        auto err = JpegLsEncode(&rgbyteCompressed[0], rgbyteCompressed.size(), &compressedLength, &rgbyteRaw[0], rgbyteOut.size(), &params, nullptr);
        Assert::IsTrue(err == ApiResult::OK);
    }
    double dwtimeEncodeComplete = getTime();

    double dwtimeDecodeStart = getTime();
    for (int i = 0; i < loopCount; ++i)
    {
        auto err = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], compressedLength, nullptr, nullptr);
        Assert::IsTrue(err == ApiResult::OK);
    }
    double dwtimeDecodeComplete = getTime();

    double bitspersample = compressedLength * 8 * 1.0 / (params.components * params.height * params.width);
    std::cout << "RoundTrip test for: " << strName << "\n\r";
    double encodeTime = (dwtimeEncodeComplete - dwtimeEncodeStart) / loopCount;
    double decodeTime = (dwtimeDecodeComplete - dwtimeDecodeStart) / loopCount;
    double symbolRate = (params.components * params.height * params.width) / (1000.0 * decodeTime);
    printf("Size:%4dx%4d, Encode time:%7.2f ms, Decode time:%7.2f ms, Bits per sample:%5.2f, Decode rate:%5.1f M/s \n\r", params.width, params.height, encodeTime, decodeTime, bitspersample, symbolRate);
    uint8_t* pbyteOut = &rgbyteOut[0];
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
    int byteCount = size2.cx * size2.cy * ccomp * ((cbit + 7)/8);
    std::vector<uint8_t> rgbyteUncompressed;

    if (!ReadFile(filename, &rgbyteUncompressed, ioffs, byteCount))
        return;

    if (cbit > 8)
    {
        FixEndian(&rgbyteUncompressed, littleEndianFile);
    }

    TestRoundTrip(filename, rgbyteUncompressed, size2, cbit, ccomp, loopCount);
}

