#pragma once

#include "../src/util.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

// Purpose: this class can read an image stored in the Portable Anymap Format (PNM).
//          The 2 binary formats P5 and P6 are supported:
//          Portable GrayMap: P5 = binary, extension = .pgm, 0-2^16 (gray scale)
//          Portable PixMap: P6 = binary, extension.ppm, range 0-2^16 (RGB)
class portable_anymap_file
{
public:
    /// <exception cref="ifstream::failure">Thrown when the input file cannot be read.</exception>
    explicit portable_anymap_file(const char* filename)
    {
        std::ifstream pnmFile;
        pnmFile.exceptions(pnmFile.exceptions() | std::ios::failbit | std::ios::eofbit);
        pnmFile.open(filename, std::ios_base::in | std::ios_base::binary);

        std::vector<int> headerInfo = read_header(pnmFile);
        if (headerInfo.size() != 4)
            throw std::istream::failure("Incorrect PNM header");

        m_componentCount = headerInfo[0] == 6 ? 3 : 1;
        m_width = headerInfo[1];
        m_height = headerInfo[2];
        m_bitsPerSample = charls::log_2(headerInfo[3] + 1);

        const int bytesPerSample = (m_bitsPerSample + 7) / 8;
        m_inputBuffer.resize(static_cast<size_t>(m_width) * m_height * bytesPerSample * m_componentCount);
        pnmFile.read(reinterpret_cast<char*>(m_inputBuffer.data()), m_inputBuffer.size());
    }

    int width() const noexcept
    {
        return m_width;
    }

    int height() const noexcept
    {
        return m_height;
    }

    int component_count() const noexcept
    {
        return m_componentCount;
    }

    int bits_per_sample() const noexcept
    {
        return m_bitsPerSample;
    }

    const std::vector<uint8_t>& image_data() const noexcept
    {
        return m_inputBuffer;
    }

private:
    static std::vector<int> read_header(std::istream& pnmFile)
    {
        std::vector<int> readValues;

        const auto first = static_cast<char>(pnmFile.get());

        // All portable anymap format (PNM) start with the character P.
        if (first != 'P')
            throw std::istream::failure("Missing P");

        while (readValues.size() < 4)
        {
            std::string bytes;
            std::getline(pnmFile, bytes);
            std::stringstream line(bytes);

            while (readValues.size() < 4)
            {
                int value = -1;
                line >> value;
                if (value <= 0)
                    break;

                readValues.push_back(value);
            }
        }
        return readValues;
    }

    int m_componentCount;
    int m_width;
    int m_height;
    int m_bitsPerSample;
    std::vector<uint8_t> m_inputBuffer;
};
