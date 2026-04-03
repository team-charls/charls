// SPDX-FileCopyrightText: © 2010 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "util.hpp"

#include "support/portable_anymap_file.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <vector>

using std::byte;
using std::cout;
using std::ifstream;
using std::ios;
using std::milli;
using std::ofstream;
using std::swap;
using std::vector;
using namespace charls;
using namespace charls::support;


namespace {

MSVC_WARNING_SUPPRESS(26497) // cannot be marked constexpr, check must be executed at runtime.

bool is_machine_little_endian() noexcept
{
    constexpr int a = 0xFF000001; // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
    const auto* chars{reinterpret_cast<const char*>(&a)}; //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return chars[0] == 0x01;
}

MSVC_WARNING_UNSUPPRESS()

} // namespace


ofstream open_output_stream(const char* filename)
{
    static constexpr ios::openmode mode_output{ios::out | ios::binary};

    ofstream stream;
    stream.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    stream.open(filename, mode_output);

    return stream;
}


void fix_endian(vector<byte>* buffer, const bool little_endian_data) noexcept
{
    if (little_endian_data == is_machine_little_endian())
        return;

    for (size_t i{}; i < buffer->size() - 1; i += 2)
    {
        swap((*buffer)[i], (*buffer)[i + 1]);
    }
}


vector<byte> read_file(const char* filename)
try
{
    ifstream input;
    input.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    input.open(filename, ios::in | ios::binary);

    input.seekg(0, ios::end);
    const auto byte_count_file{static_cast<size_t>(input.tellg())};
    input.seekg(0, ios::beg);

    vector<byte> buffer(byte_count_file);
    read(input, buffer);

    return buffer;
}
catch (const std::ifstream::failure&)
{
    cout << "Failed to open/read file: " << std::filesystem::absolute(filename) << "\n";
    throw;
}

void write_file(const char* filename, const void* data, const size_t size)
{
    ofstream output;
    output.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    output.open(filename, ios::out | ios::binary);
    output.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    output.close(); // close explicitly to get exception on failure.
}

