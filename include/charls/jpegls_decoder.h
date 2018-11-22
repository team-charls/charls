#pragma once

#include "charls.h"

#include <vector>
#include <cstddef>
#include <filesystem>

// WARNING: THESE CLASSES ARE NOT FINAL AND THEIR DESIGN AND API MAY CHANGE

namespace charls {

struct metadata_info_t
{
    int32_t width;
    int32_t height;
    int32_t bits_per_sample;
    int32_t component_count;
};


class decoder
{
public:
    decoder() = default;

    void read_header(const void* source, const size_t source_size_bytes)
    {
        std::error_code ec;
        read_header(source, source_size_bytes, ec);
        if (ec)
            throw jpegls_error(ec);
    }

    void read_header(const void* source, const size_t source_size_bytes, std::error_code& error) noexcept
    {
        error = JpegLsReadHeader(source, source_size_bytes, &params_, nullptr);
        if (error)
            return;

        source_ = source;
        source_size_bytes_ = source_size_bytes;
        metadata_ = { params_.width, params_.height, params_.bitsPerSample, params_.components };
    }

    const metadata_info_t& metadata_info() const
    {
        return metadata_;
    }

    std::vector<std::byte> decode() const
    {
        std::vector<std::byte> buffer(params_.width * params_.height * params_.components * (params_.bitsPerSample <= 8 ? 1 : 2));

        std::error_code error;
        decode(buffer.data(), buffer.size(), error);
        if (error)
            throw jpegls_error(error);

        return buffer;
    }

    std::vector<std::byte> decode(std::error_code& error) const
    {
        std::vector<std::byte> buffer(params_.width * params_.height * params_.components * (params_.bitsPerSample <= 8 ? 1 : 2));
        decode(buffer.data(), buffer.size(), error);
        return buffer;
    }

    void decode(void* destination, const size_t destination_size_bytes) const
    {
        std::error_code error;
        decode(destination, destination_size_bytes, error);
        if (error)
            throw jpegls_error(error);
    }

    void decode(void* destination, const size_t destination_size_bytes, std::error_code& error) const noexcept
    {
        error = JpegLsDecode(destination, destination_size_bytes, source_, source_size_bytes_, &params_, nullptr);
    }

private:
    const void* source_{};
    size_t source_size_bytes_{};
    JlsParameters params_{};
    metadata_info_t metadata_{};
};

}
