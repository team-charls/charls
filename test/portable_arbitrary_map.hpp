// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace charls_test {

// Purpose: This class can read an image stored in the Portable Arbitrary Map Format (PAM).
// Remark: Minimal checking is done on the input file: it is assumed that the file is well formed.
class portable_arbitrary_map final
{
public:
    /// <exception cref="ifstream::failure">Thrown when the input file cannot be read.</exception>
    explicit portable_arbitrary_map(const char* filename)
    {
        std::ifstream file;
        file.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        file.open(filename, std::ios_base::in | std::ios_base::binary);

        std::string magic;
        file >> magic;
        if (magic != "P7")
            throw std::runtime_error("Not a PAM file (expected P7)");

        // Parse header
        std::string token;
        while (file >> token && token != "ENDHDR")
        {
            if (token == "WIDTH")
                file >> width_;
            else if (token == "HEIGHT")
                file >> height_;
            else if (token == "DEPTH")
                file >> component_count_;
            else if (token == "MAXVAL")
                file >> max_value_;
            else
            {
                // Skip unknown tokens and their values (rest of the line).
                std::string ignored_line;
                std::getline(file, ignored_line);
            }
        }
        file.ignore();

        bits_per_sample_ = log_2(max_value_ + 1);

        const int bytes_per_sample{(bits_per_sample_ + 7) / 8};
        image_data_.resize(static_cast<size_t>(width_) * height_ * bytes_per_sample * component_count_);
        file.read(reinterpret_cast<char*>(image_data_.data()), static_cast<std::streamsize>(image_data_.size()));
    }

    [[nodiscard]]
    uint32_t width() const noexcept
    {
        return width_;
    }

    [[nodiscard]]
    uint32_t height() const noexcept
    {
        return height_;
    }

    [[nodiscard]]
    int32_t component_count() const noexcept
    {
        return component_count_;
    }

    [[nodiscard]]
    int32_t bits_per_sample() const noexcept
    {
        return bits_per_sample_;
    }

    [[nodiscard]]
    const std::vector<std::byte>& image_data() const noexcept
    {
        return image_data_;
    }

private:
    [[nodiscard]]
    static constexpr int32_t log_2(const int32_t n) noexcept
    {
        int32_t x{};
        while (n > (1 << x))
        {
            ++x;
        }
        return x;
    }

    uint32_t width_{};
    uint32_t height_{};
    int32_t component_count_{};
    int32_t max_value_{};
    int32_t bits_per_sample_{};
    std::vector<std::byte> image_data_;
};

} // namespace charls_test
