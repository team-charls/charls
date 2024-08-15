// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace charls {

constexpr int32_t default_reset_threshold{64}; // Default RESET value as defined in ISO/IEC 14495-1, table C.2

constexpr int32_t minimum_component_count{1};
constexpr int32_t maximum_component_count{255};
constexpr size_t maximum_component_count_in_scan{4};
constexpr int32_t minimum_component_index{};
constexpr int32_t maximum_component_index{maximum_component_count - 1};
constexpr int32_t minimum_bits_per_sample{2};
constexpr int32_t maximum_bits_per_sample{16};
constexpr int32_t maximum_near_lossless{255};
constexpr int32_t minimum_application_data_id{};
constexpr int32_t maximum_application_data_id{15};

// The following limits for mapping tables are defined in ISO/IEC 14495-1, C.2.4.1.2, table C.4.
constexpr int32_t minimum_table_id{1};
constexpr int32_t maximum_table_id{255};
constexpr int32_t minimum_entry_size{1};
constexpr int32_t maximum_entry_size{255};

constexpr int32_t max_k_value{16}; // This is an implementation limit (theoretical limit is 32)

// ISO/IEC 14495-1, section 4.8.1 defines the SPIFF version numbers to be used for the SPIFF header in combination with
// JPEG-LS.
constexpr uint8_t spiff_major_revision_number{2};
constexpr uint8_t spiff_minor_revision_number{};

constexpr uint8_t spiff_end_of_directory_entry_type{1};

// The size of a SPIFF header when serialized to a JPEG byte stream.
constexpr size_t spiff_header_size_in_bytes{34};

// The special value to indicate that the stride should be calculated.
constexpr size_t auto_calculate_stride{};

// The size in bytes of the segment length field.
constexpr size_t segment_length_size{sizeof(uint16_t)};

// The maximum size of the data bytes that fit in a segment.
constexpr size_t segment_max_data_size{std::numeric_limits<uint16_t>::max() - segment_length_size};

// Number of bits in an int32_t data type.
constexpr size_t int32_t_bit_count{sizeof(int32_t) * 8};

} // namespace charls
