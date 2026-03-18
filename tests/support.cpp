// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "support.hpp"

#include <fstream>

using std::byte;
using std::ifstream;
using std::vector;

namespace charls::test {

[[nodiscard]]
vector<byte> read_file(const char* filename)
{
    std::ifstream input;
    input.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    input.open(filename, std::ios::in | std::ios::binary);

    input.seekg(0, std::ios::end);
    const auto byte_count_file{static_cast<size_t>(input.tellg())};
    input.seekg(0, std::ios::beg);

    vector<byte> buffer(byte_count_file);
    input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));

    return buffer;
}

} // namespace charls::test
