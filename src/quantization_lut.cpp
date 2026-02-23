// SPDX-FileCopyrightText: © 2023 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "quantization_lut.hpp"

namespace charls {

using std::vector;

const vector<int8_t>& quantization_lut_lossless_16()
{
    static const vector<int8_t> lut{[] {
        constexpr int32_t max_val{calculate_maximum_sample_value(16)};
        constexpr auto preset{compute_default(max_val, 0)};
        constexpr int32_t range{preset.maximum_sample_value + 1};

        vector<int8_t> table(static_cast<size_t>(range) * 2);
        for (size_t i{}; i != table.size(); ++i)
        {
            table[i] = quantize_gradient_org(static_cast<int32_t>(i) - range, preset.threshold1, preset.threshold2,
                                             preset.threshold3);
        }
        return table;
    }()};

    return lut;
}

} // namespace charls
