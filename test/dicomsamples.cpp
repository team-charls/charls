// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "dicomsamples.h"
#include "util.h"

#include <array>
#include <iostream>
#include <vector>

using std::array;
using std::cout;
using std::error_code;
using std::vector;

namespace {

bool contains_string(const uint8_t* container, const uint8_t* bytes_to_find, const size_t bytes_length) noexcept
{
    for (size_t j = 0; j < bytes_length; ++j)
    {
        if (bytes_to_find[j] != container[j])
            return false;
    }

    return true;
}

int find_string(vector<uint8_t>& container, const uint8_t* bytes_to_find, const size_t bytes_length) noexcept
{
    for (size_t i = 0; i < container.size() - bytes_length; ++i)
    {
        if (contains_string(&container[i], bytes_to_find, bytes_length))
            return static_cast<int>(i);
    }
    return -1;
}

// ReSharper disable CppDeprecatedEntity
DISABLE_DEPRECATED_WARNING

void test_dicom_sample_image(const char* name)
{
    vector<uint8_t> data = read_file(name);

    const array<uint8_t, 8> pixel_data_start = {0x00, 0x00, 0x01, 0x00, 0xFF, 0xD8, 0xFF, 0xF7};

    const int offset = find_string(data, pixel_data_start.data(), pixel_data_start.size());

    data.erase(data.begin(), data.begin() + offset - 4);

    // remove the DICOM fragment headers (in the concerned images they occur every 64k)
    for (unsigned int i = 0; i < data.size(); i += 64 * 1024)
    {
        data.erase(data.begin() + i, data.begin() + i + 8);
    }

    JlsParameters params{};
    error_code error = JpegLsReadHeader(data.data(), data.size(), &params, nullptr);
    assert::is_true(!error);

    vector<uint8_t> data_unc;
    data_unc.resize(static_cast<size_t>(params.stride) * params.height);

    error = JpegLsDecode(data_unc.data(), data_unc.size(), data.data(), data.size(), nullptr, nullptr);
    assert::is_true(!error);
    cout << ".";
}

// ReSharper restore CppDeprecatedEntity
RESTORE_DEPRECATED_WARNING

} // namespace


void test_dicom_wg4_images()
{
    test_dicom_sample_image("test/compsamples_jpegls/IMAGES/JLSL/XA1_JLSL");
    test_dicom_sample_image("test/compsamples_jpegls/IMAGES/JLSL/CT2_JLSL");
    test_dicom_sample_image("test/compsamples_jpegls/IMAGES/JLSL/MG1_JLSL");
    test_dicom_sample_image("test/compsamples_jpegls/IMAGES/JLSL/MR1_JLSL");
    test_dicom_sample_image("test/compsamples_jpegls/IMAGES/JLSL/MR2_JLSL");
    test_dicom_sample_image("test/compsamples_jpegls/IMAGES/JLSL/MR3_JLSL");
    test_dicom_sample_image("test/compsamples_jpegls/IMAGES/JLSL/MR4_JLSL");
    test_dicom_sample_image("test/compsamples_jpegls/IMAGES/JLSL/NM1_JLSL");
    test_dicom_sample_image("test/compsamples_jpegls/IMAGES/JLSL/RG1_JLSL");
    test_dicom_sample_image("test/compsamples_jpegls/IMAGES/JLSL/RG2_JLSL");
    test_dicom_sample_image("test/compsamples_jpegls/IMAGES/JLSL/RG3_JLSL");
    test_dicom_sample_image("test/compsamples_jpegls/IMAGES/JLSL/SC1_JLSL");
}
