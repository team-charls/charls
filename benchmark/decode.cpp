// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <benchmark/benchmark.h>

#include "../include/charls/charls.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#pragma warning(disable : 26409) // Avoid calling new explicitly (triggered by BENCHMARK macro)

using namespace charls;
using std::byte;
using std::ifstream;
using std::ios;
using std::vector;

template<typename Container>
void read(std::istream& input, Container& destination)
{
    input.read(reinterpret_cast<char*>(destination.data()), static_cast<std::streamsize>(destination.size()));
}

vector<byte> read_file(const char* filename, long offset = 0, size_t bytes = 0)
try
{
    ifstream input;
    input.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    input.open(filename, ios::in | ios::binary);

    input.seekg(0, ios::end);
    const auto byte_count_file{static_cast<int>(input.tellg())};
    input.seekg(offset, ios::beg);

    if (offset < 0)
    {
        offset = static_cast<long>(byte_count_file - bytes);
    }
    if (bytes == 0)
    {
        bytes = static_cast<size_t>(byte_count_file) - offset;
    }

    vector<byte> buffer(bytes);
    read(input, buffer);

    return buffer;
}
catch (const std::ifstream::failure&)
{
    std::cout << "Failed to open/read file: " << std::filesystem::absolute(filename) << "\n";
    throw;
}


static void bm_decode(benchmark::State& state)
{
    const auto source{read_file("d:/benchmark-test-image.jls")};

    // Pre-allocate the destination outside the measurement loop.
    // std::vector initializes its elements and this step needs to be excluded from the measurement.
    vector<byte> destination(jpegls_decoder{source, true}.get_destination_size());

    for (const auto _ : state)
    {
        jpegls_decoder decoder(source.data(), source.size());
        decoder.decode(destination);
    }
}
BENCHMARK(bm_decode);
