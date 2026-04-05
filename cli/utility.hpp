// SPDX-FileCopyrightText: © 2010 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/charls.hpp>

#include <fstream>
#include <istream>
#include <ostream>
#include <vector>
#include <cstddef>
#include <filesystem>

namespace charls::cli {

[[nodiscard]]
std::ofstream open_output_stream(const std::filesystem::path& filename);

void fix_endian(std::vector<std::byte>* buffer, bool little_endian_data) noexcept;

[[nodiscard]]
std::vector<std::byte> read_file(const std::filesystem::path& filename);

void write_file(const std::filesystem::path& filename, const void* data, size_t size);

/// <summary>
/// Computes how many bytes are needed to hold the number of bits.
/// </summary>
[[nodiscard]]
constexpr uint32_t bit_to_byte_count(const int32_t bit_count) noexcept
{
    return static_cast<uint32_t>((bit_count + 7) / 8);
}

template<typename Container>
void read(std::istream& input, Container& destination)
{
    input.read(reinterpret_cast<char*>(destination.data()), static_cast<std::streamsize>(destination.size()));
}

template<typename Container>
void write(std::ostream& output, const Container& source, const size_t size)
{
    output.write(reinterpret_cast<const char*>(source.data()), static_cast<std::streamsize>(size));
}

[[nodiscard]]
std::ifstream open_input_stream(const std::filesystem::path& filename);

[[nodiscard]]
std::vector<int> read_pnm_header(std::istream& pnm_file);

}
