// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "compare.hpp"

#include "utility.hpp"

#include <iostream>

using std::byte;
using std::cout;
using std::ifstream;
using std::ios;
using std::vector;

namespace charls::cli {

bool compare_netpbm(const std::filesystem::path& filename1, const std::filesystem::path& filename2)
{
    constexpr ios::openmode mode_input{ios::in | ios::binary};
    ifstream pnm_file1(filename1, mode_input);
    ifstream pnm_file2(filename2, mode_input);

    const auto header1{read_pnm_header(pnm_file1)};
    if (header1.size() != 4)
    {
        cout << "Cannot read header from input file 1\n";
        return false;
    }

    const auto header2{read_pnm_header(pnm_file2)};
    if (header2.size() != 4)
    {
        cout << "Cannot read header from input file 2\n";
        return false;
    }

    if (header1[0] != header2[0])
    {
        cout << "Header type " << header1[0] << " is not equal with type " << header2[0] << "\n";
        return false;
    }

    const auto width{static_cast<size_t>(header1[1])};
    if (width != static_cast<size_t>(header2[1]))
    {
        cout << "Width " << width << " is not equal with width " << header2[1] << "\n";
        return false;
    }

    const auto height{static_cast<size_t>(header1[2])};
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
    const size_t bytes_per_sample{header1[3] > 255 ? 2U : 1U};

    const size_t byte_count{width * height * bytes_per_sample};
    vector<byte> bytes1(byte_count);
    vector<byte> bytes2(byte_count);

    read(pnm_file1, bytes1);
    read(pnm_file2, bytes2);

    for (size_t x{}; x != height; ++x)
    {
        for (size_t y{}; y < width; y += bytes_per_sample)
        {
            if (bytes_per_sample == 1)
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

} // namespace charls::cli
