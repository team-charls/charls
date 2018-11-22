#pragma once

#include "charls.h"

#include <vector>
#include <cstddef>

// WARNING: THESE CLASSES ARE NOT FINAL AND THEIR DESIGN AND API MAY CHANGE

namespace charls {

struct metadata
{
    int32_t width;
    int32_t height;
    int32_t bits_per_sample;
    int32_t component_count;
};

class jpegls_encoder final
{
public:
    void source(const void* source, size_t source_size_bytes, const metadata& metadata) noexcept
    {
        source_ = source,
        source_size_bytes_ = source_size_bytes;
        metadata_ = metadata;
    }

    void interleave_mode(InterleaveMode interleave_mode) noexcept
    {
        interleave_mode_ = interleave_mode;
    }

    void allowed_lossy_error(int value) noexcept
    {
        allowed_lossy_error_ = value;
    }

    std::vector<std::byte> encode()
    {
        // Assume that compressed pixels are smaller or equal to uncompressed pixels and reserve some room for JPEG header.
        const size_t encoded_buffer_size = source_size_bytes_ + 1024;

        std::vector<std::byte> buffer(encoded_buffer_size);
        buffer.resize(encode(buffer.data(), buffer.size()));

        return buffer;
    }

    size_t encode(void* destination, const size_t destination_size_bytes)
    {
        std::error_code error;
        const size_t final_size = encode(destination, destination_size_bytes, error);
        if (error)
            throw jpegls_error(error);

        return final_size;
    }

    size_t encode(void* destination, const size_t destination_size_bytes, std::error_code& error) noexcept
    {
        size_t bytes_written;
        JlsParameters parameters
        {
            metadata_.width,
            metadata_.height,
            metadata_.bits_per_sample,
            0,
            metadata_.component_count,
            allowed_lossy_error_,
            interleave_mode_
        };

        error = JpegLsEncode(destination, destination_size_bytes, &bytes_written,
                             source_, source_size_bytes_, &parameters, nullptr);
        return bytes_written;
    }

private:
    InterleaveMode interleave_mode_{InterleaveMode::None};
    int allowed_lossy_error_{};

    const void* source_{};
    size_t source_size_bytes_{};
    metadata metadata_{};
};

} // namespace charls
