#pragma once

#include "../src/util.h"

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <ios>

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

        std::vector<int> header_info = read_header(pnm_file);
        if (header_info.size() != 4)
            throw std::ios_base::failure("Incorrect PNM header");

        component_count_ = header_info[0] == 6 ? 3 : 1;
        width_ = header_info[1];
        height_ = header_info[2];
        bits_per_sample_ = charls::log_2(header_info[3] + 1);

        const int bytes_per_sample = (bits_per_sample_ + 7) / 8;
        input_buffer_.resize(static_cast<size_t>(width_) * height_ * bytes_per_sample * component_count_);
        pnm_file.read(reinterpret_cast<char*>(input_buffer_.data()), input_buffer_.size());
    }

    int width() const noexcept
    {
        return width_;
    }

    int height() const noexcept
    {
        return height_;
    }

    int component_count() const noexcept
    {
        return component_count_;
    }

    int bits_per_sample() const noexcept
    {
        return bits_per_sample_;
    }

    std::vector<uint8_t>& image_data() noexcept
    {
        return input_buffer_;
    }

private:
    static std::vector<int> read_header(std::istream& pnm_file)
    {
        std::vector<int> result;

        const auto first = static_cast<char>(pnm_file.get());

        // All portable anymap format (PNM) start with the character P.
        if (first != 'P')
            throw std::istream::failure("Missing P");

        while (result.size() < 4)
        {
            std::string bytes;
            std::getline(pnm_file, bytes);
            std::stringstream line(bytes);

            while (result.size() < 4)
            {
                int value = -1;
                line >> value;
                if (value <= 0)
                    break;

                result.push_back(value);
            }
        }
        return result;
    }

    int component_count_;
    int width_;
    int height_;
    int bits_per_sample_;
    std::vector<uint8_t> input_buffer_;
};

} // namespace charls_test
