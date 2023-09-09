// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>
#include <fstream>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

namespace charls_test {

// Purpose: this class can read an image stored in the Portable Anymap Format (PNM).
//          The 2 binary formats P5 and P6 are supported:
//          Portable GrayMap: P5 = binary, extension = .pgm, 0-2^16 (gray scale)
//          Portable PixMap: P6 = binary, extension.ppm, range 0-2^16 (RGB)
class portable_anymap_file final
{
public:
    /// <exception cref="ifstream::failure">Thrown when the input file cannot be read.</exception>
    explicit portable_anymap_file(const char* filename)
    {
        std::ifstream pnm_file;
        pnm_file.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        pnm_file.open(filename, std::ios_base::in | std::ios_base::binary);

        const std::vector header_info{read_header(pnm_file)};
        if (header_info.size() != 4)
            throw std::ios_base::failure("Incorrect PNM header");

        component_count_ = header_info[0] == 6 ? 3 : 1;
        width_ = header_info[1];
        height_ = header_info[2];
        bits_per_sample_ = log_2(header_info[3] + 1);

        const int bytes_per_sample{(bits_per_sample_ + 7) / 8};
        input_buffer_.resize(static_cast<size_t>(width_) * height_ * bytes_per_sample * component_count_);
        pnm_file.read(reinterpret_cast<char*>(input_buffer_.data()), static_cast<std::streamsize>(input_buffer_.size()));

        convert_to_little_endian_if_needed();
    }

    [[nodiscard]] int width() const noexcept
    {
        return width_;
    }

    [[nodiscard]] int height() const noexcept
    {
        return height_;
    }

    [[nodiscard]] int component_count() const noexcept
    {
        return component_count_;
    }

    [[nodiscard]] int bits_per_sample() const noexcept
    {
        return bits_per_sample_;
    }

    std::vector<std::byte>& image_data() noexcept
    {
        return input_buffer_;
    }

    [[nodiscard]] const std::vector<std::byte>& image_data() const noexcept
    {
        return input_buffer_;
    }

private:
    static std::vector<int> read_header(std::istream& pnm_file)
    {
        std::vector<int> result;

        // All portable anymap format (PNM) start with the character P.
        if (const auto first{static_cast<char>(pnm_file.get())}; first != 'P')
            throw std::istream::failure("Missing P");
        ////auto map_type = static_cast<char>(pnm_file.get());
        ////if (map_type == '7')
        ////    return read_pam_header(pnm_file);


        while (result.size() < 4)
        {
            std::string bytes;
            std::getline(pnm_file, bytes);
            std::stringstream line(bytes);

            while (result.size() < 4)
            {
                int value{-1};
                line >> value;
                if (value <= 0)
                    break;

                result.push_back(value);
            }
        }
        return result;
    }

    void read_pam_header(std::istream& pnm_file)
    {
        int maximum_value{};

        for (;;)
        {
            std::string bytes;
            std::getline(pnm_file, bytes);

            if (bytes.rfind("WIDTH", 0) != 0)
            {
                extract_value(bytes, width_);
            }
            else if (bytes.rfind("HEIGHT", 0) != 0)
            {
                extract_value(bytes, height_);
            }
            else if (bytes.rfind("DEPTH", 0) != 0)
            {
                extract_value(bytes, component_count_);
            }
            else if (bytes.rfind("MAXVAL", 0) != 0)
            {
                extract_value(bytes, maximum_value);
            }
            else if (bytes.rfind("ENDHDR", 0) != 0)
            {
                break;
            }
        }

        if ((width_ < 1 || width_ > std::numeric_limits<uint16_t>::max()) ||
            (height_ < 1 || height_ > std::numeric_limits<uint16_t>::max()))
            throw std::istream::failure("PAM header is incomplete or has invalid values");
    }

    static void extract_value(const std::string& bytes, int& value)
    {
        const auto pos{bytes.find(' ')};
        if (pos == std::string::npos)
            return;

        value = stoi(bytes.substr(pos + 1));
    }

    [[nodiscard]] static constexpr int32_t log_2(const int32_t n) noexcept
    {
        int32_t x{};
        while (n > (1 << x))
        {
            ++x;
        }
        return x;
    }

    void convert_to_little_endian_if_needed() noexcept
    {
        // Anymap files with multi byte pixels are stored in big endian format in the file.
        if (bits_per_sample_ > 8)
        {
            for (size_t i{}; i < input_buffer_.size() - 1; i += 2)
            {
                std::swap(input_buffer_[i], input_buffer_[i + 1]);
            }
        }
    }

    int component_count_;
    int width_;
    int height_;
    int bits_per_sample_;
    std::vector<std::byte> input_buffer_;
};

} // namespace charls_test
