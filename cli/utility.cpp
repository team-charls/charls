// SPDX-FileCopyrightText: © 2010 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "utility.hpp"

#include "support/portable_anymap_file.hpp"

#include <chrono>

using std::byte;
using std::ifstream;
using std::ios;
using std::milli;
using std::ofstream;
using std::runtime_error;
using std::string;
using std::stringstream;
using std::swap;
using std::vector;
using std::filesystem::path;
using namespace charls::support;

namespace charls::cli {

namespace {

MSVC_WARNING_SUPPRESS(26497) // cannot be marked constexpr, check must be executed at runtime.

bool is_machine_little_endian() noexcept
{
    constexpr int a = 0xFF000001; // NOLINT(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
    const auto* chars{reinterpret_cast<const char*>(&a)}; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return chars[0] == 0x01;
}

MSVC_WARNING_UNSUPPRESS()

} // namespace


void fix_endian(vector<byte>* buffer, const bool little_endian_data) noexcept
{
    if (little_endian_data == is_machine_little_endian())
        return;

    for (size_t i{}; i < buffer->size() - 1; i += 2)
    {
        swap((*buffer)[i], (*buffer)[i + 1]);
    }
}


ofstream open_output_stream(const path& filename)
try
{
    ofstream stream;
    stream.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    stream.open(filename, ios::out | ios::binary);

    return stream;
}
catch (const std::ifstream::failure&)
{
    throw std::runtime_error("Failed to open file: " + filename.string());
}


ifstream open_input_stream(const path& filename)
try
{
    ifstream stream;
    stream.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    stream.open(filename, ios::in | ios::binary);

    return stream;
}
catch (const std::ifstream::failure&)
{
    throw std::runtime_error("Failed to open file: " + filename.string());
}


vector<byte> read_file(const path& filename)
{
    ifstream input{open_input_stream(filename)};

    try
    {
        input.seekg(0, ios::end);
        const auto byte_count_file{static_cast<size_t>(input.tellg())};
        input.seekg(0, ios::beg);

        vector<byte> buffer(byte_count_file);
        read(input, buffer);

        return buffer;
    }
    catch (const std::ifstream::failure&)
    {
        throw runtime_error("Failed to read from file: " + std::filesystem::absolute(filename).string());
    }
}


void write_file(const path& filename, const void* data, const size_t size)
{
    ofstream output;
    output.exceptions(ios::eofbit | ios::failbit | ios::badbit);
    output.open(filename, ios::out | ios::binary);
    output.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    output.close(); // close explicitly to get exception on failure.
}


vector<int> read_pnm_header(std::istream& pnm_file)
{
    vector<int> read_values;

    // All portable anymap format (PNM) start with the character P.
    if (const auto first{static_cast<char>(pnm_file.get())}; first != 'P')
        return read_values;

    while (read_values.size() < 4)
    {
        string bytes;
        getline(pnm_file, bytes);
        stringstream line(bytes);

        while (read_values.size() < 4)
        {
            int value{-1};
            line >> value;
            if (value <= 0)
                break;

            read_values.push_back(value);
        }
    }
    return read_values;
}

} // namespace charls::cli
