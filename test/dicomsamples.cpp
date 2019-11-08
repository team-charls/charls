// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "dicomsamples.h"
#include "util.h"

#include <iostream>
#include <vector>
#include <array>

using std::cout;
using std::vector;
using std::array;
using std::error_code;

namespace
{

bool ContainsString(const uint8_t* container, const uint8_t* bytesToFind, size_t bytesLength) noexcept
{
    for (size_t j = 0; j < bytesLength; ++j)
    {
        if (bytesToFind[j] != container[j])
            return false;
    }

    return true;
}

int FindString(vector<uint8_t>& container, const uint8_t* bytesToFind, size_t bytesLength) noexcept
{
    for (size_t i = 0; i < container.size() - bytesLength; ++i)
    {
        if (ContainsString(&container[i], bytesToFind, bytesLength))
            return static_cast<int>(i);
    }
    return -1;
}


void TestDicomSampleImage(const char* name)
{
    vector<uint8_t> data = ReadFile(name);

    const array<uint8_t, 8> pixelDataStart = {0x00, 0x00, 0x01, 0x00, 0xFF, 0xD8, 0xFF, 0xF7};

    const int offset = FindString(data, pixelDataStart.data(), pixelDataStart.size());

    data.erase(data.begin(), data.begin() + offset - 4);

    // remove the DICOM fragment headers (in the concerned images they occur every 64k)
    for (unsigned int i =  0; i < data.size(); i+= 64 * 1024)
    {
        data.erase(data.begin() + i, data.begin() + i + 8);
    }

    JlsParameters params{};
    error_code error = JpegLsReadHeader(data.data(), data.size(), &params, nullptr);
    Assert::IsTrue(!error);

    vector<uint8_t> dataUnc;
    dataUnc.resize(static_cast<size_t>(params.stride) * params.height);

    error = JpegLsDecode(dataUnc.data(), dataUnc.size(), data.data(), data.size(), nullptr, nullptr);
    Assert::IsTrue(!error);
    cout << ".";
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
