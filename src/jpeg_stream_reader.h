// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/charls_legacy.h>
#include <charls/public_types.h>

#include "coding_parameters.h"

#include <cstdint>
#include <vector>

namespace charls {

enum class JpegMarkerCode : uint8_t;

// Purpose: minimal implementation to read a JPEG byte stream.
class JpegStreamReader final
{
public:
    explicit JpegStreamReader(ByteStreamInfo byteStreamInfo) noexcept;

    const charls::frame_info& frame_info() const noexcept
    {
        return frame_info_;
    }

    const coding_parameters& parameters() const noexcept
    {
        return parameters_;
    }

    const jpegls_pc_parameters& GetCustomPreset() const noexcept
    {
        return preset_coding_parameters_;
    }

    void Read(ByteStreamInfo rawPixels, uint32_t stride);
    void ReadHeader(spiff_header* header = nullptr, bool* spiff_header_found = nullptr);

    void SetOutputBgr(const bool value) noexcept
    {
        parameters_.output_bgr = value;
    }

    void SetRect(const JlsRect& rect) noexcept
    {
        rect_ = rect;
    }

    void ReadStartOfScan();
    uint8_t ReadByte();

private:
    void SkipByte();
    int ReadUInt16();
    uint32_t ReadUInt32();
    int32_t ReadSegmentSize();
    void ReadNBytes(std::vector<char>& destination, int byteCount);
    void ReadNextStartOfScan();
    JpegMarkerCode ReadNextMarkerCode();
    void ValidateMarkerCode(JpegMarkerCode markerCode) const;

    int ReadMarkerSegment(JpegMarkerCode markerCode, int32_t segmentSize, spiff_header* header = nullptr, bool* spiff_header_found = nullptr);
    int ReadSpiffDirectoryEntry(JpegMarkerCode markerCode, int32_t segmentSize);
    int ReadStartOfFrameSegment(int32_t segmentSize);
    static int ReadComment() noexcept;
    int ReadPresetParametersSegment(int32_t segmentSize);
    int TryReadApplicationData8Segment(int32_t segmentSize, spiff_header* header, bool* spiff_header_found);
    int TryReadSpiffHeaderSegment(OUT_ spiff_header& header, OUT_ bool& spiff_header_found);

    int TryReadHPColorTransformSegment();
    void AddComponent(uint8_t componentId);
    void CheckParameterCoherent() const;

    enum class state
    {
        before_start_of_image,
        header_section,
        spiff_header_section,
        image_section,
        frame_section,
        scan_section,
        bit_stream_section
    };

    ByteStreamInfo byteStream_;
    charls::frame_info frame_info_{};
    coding_parameters parameters_{};
    jpegls_pc_parameters preset_coding_parameters_{};
    JlsRect rect_{};
    std::vector<uint8_t> componentIds_;
    state state_{};
};

} // namespace charls
