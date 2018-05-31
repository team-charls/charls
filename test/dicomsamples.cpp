//
// (C) CharLS Team 2016, all rights reserved. See the accompanying "License.txt" for licensed use.
//

#include "dicomsamples.h"
#include "util.h"
#include "../src/charls.h"
#include <iostream>
#include <vector>


#define COUNT(x) (sizeof(x)/sizeof((x)[0]))


namespace
{

bool contains_string(const uint8_t* container, const uint8_t* bytesToFind, unsigned int bytesLength) noexcept
{
    for (unsigned int j = 0; j < bytesLength; ++j)
    {
        if (bytesToFind[j] != container[j])
            return false;
    }

    return true;
}

int findstring(std::vector<uint8_t>& container, const uint8_t* bytesToFind, unsigned int bytesLength)
{
    for (unsigned int i=0; i < container.size() - bytesLength; ++i)
    {
        if (contains_string(&container[i], bytesToFind, bytesLength))
            return i;
    }
    return -1;
}


void TestDicomSampleImage(const char* name)
{
    std::vector<uint8_t> data;
    const bool success = ReadFile(name, &data, 9);

    Assert::IsTrue(success);

    const uint8_t pixeldataStart[] =  { 0x00, 0x00, 0x01, 0x00, 0xFF, 0xD8, 0xFF, 0xF7 };

    const int offset = findstring(data, pixeldataStart, COUNT(pixeldataStart));

    data.erase(data.begin(), data.begin() + offset - 4);

    // remove the DICOM fragment headers (in the concerned images they occur every 64k)
    for (unsigned int i =  0; i < data.size(); i+= 64 * 1024)
    {
        data.erase(data.begin() + i, data.begin() + i + 8);
    }

    JlsParameters params{};
    auto error = JpegLsReadHeader(data.data(), data.size(), &params, nullptr);
    Assert::IsTrue(error == charls::ApiResult::OK);

    std::vector<uint8_t> dataUnc;
    dataUnc.resize(params.stride * params.height);

    error = JpegLsDecode(dataUnc.data(), dataUnc.size(), &data[0], data.size(), nullptr, nullptr);
    Assert::IsTrue(error == charls::ApiResult::OK);
    std::cout << ".";
}

} // namespace


void TestDicomWG4Images()
{
    TestDicomSampleImage("test/compsamples_jpegls/IMAGES/JLSL/XA1_JLSL");
    TestDicomSampleImage("test/compsamples_jpegls/IMAGES/JLSL/CT2_JLSL");
    TestDicomSampleImage("test/compsamples_jpegls/IMAGES/JLSL/MG1_JLSL");
    TestDicomSampleImage("test/compsamples_jpegls/IMAGES/JLSL/MR1_JLSL");
    TestDicomSampleImage("test/compsamples_jpegls/IMAGES/JLSL/MR2_JLSL");
    TestDicomSampleImage("test/compsamples_jpegls/IMAGES/JLSL/MR3_JLSL");
    TestDicomSampleImage("test/compsamples_jpegls/IMAGES/JLSL/MR4_JLSL");
    TestDicomSampleImage("test/compsamples_jpegls/IMAGES/JLSL/NM1_JLSL");
    TestDicomSampleImage("test/compsamples_jpegls/IMAGES/JLSL/RG1_JLSL");
    TestDicomSampleImage("test/compsamples_jpegls/IMAGES/JLSL/RG2_JLSL");
    TestDicomSampleImage("test/compsamples_jpegls/IMAGES/JLSL/RG3_JLSL");
    TestDicomSampleImage("test/compsamples_jpegls/IMAGES/JLSL/SC1_JLSL");
}
