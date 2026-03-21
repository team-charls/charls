// SPDX-FileCopyrightText: © 2026 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "../src/lossless_traits.hpp"
#include "../src/sample_traits.hpp"

namespace charls::test {

TEST(sample_traits_test, basics)
{
    // extract_sample: scalar types are identity.
    static_assert(std::is_same_v<extract_sample<uint8_t>::type, uint8_t>);
    static_assert(std::is_same_v<extract_sample<uint16_t>::type, uint16_t>);

    // extract_sample: compound types extract the element type.
    static_assert(std::is_same_v<extract_sample<pair<uint8_t>>::type, uint8_t>);
    static_assert(std::is_same_v<extract_sample<triplet<uint8_t>>::type, uint8_t>);
    static_assert(std::is_same_v<extract_sample<quad<uint8_t>>::type, uint8_t>);
    static_assert(std::is_same_v<extract_sample<pair<uint16_t>>::type, uint16_t>);
    static_assert(std::is_same_v<extract_sample<triplet<uint16_t>>::type, uint16_t>);
    static_assert(std::is_same_v<extract_sample<quad<uint16_t>>::type, uint16_t>);

    // sample_traits_t: scalar lossless_traits are identity.
    static_assert(std::is_same_v<sample_traits_t<lossless_traits<uint8_t, 8>>, lossless_traits<uint8_t, 8>>);
    static_assert(std::is_same_v<sample_traits_t<lossless_traits<uint16_t, 16>>, lossless_traits<uint16_t, 16>>);
    static_assert(std::is_same_v<sample_traits_t<lossless_traits<uint16_t, 12>>, lossless_traits<uint16_t, 12>>);

    // sample_traits_t: compound lossless_traits map to scalar.
    static_assert(std::is_same_v<sample_traits_t<lossless_traits<pair<uint8_t>, 8>>, lossless_traits<uint8_t, 8>>);
    static_assert(std::is_same_v<sample_traits_t<lossless_traits<triplet<uint8_t>, 8>>, lossless_traits<uint8_t, 8>>);
    static_assert(std::is_same_v<sample_traits_t<lossless_traits<quad<uint8_t>, 8>>, lossless_traits<uint8_t, 8>>);
    static_assert(std::is_same_v<sample_traits_t<lossless_traits<pair<uint16_t>, 16>>, lossless_traits<uint16_t, 16>>);
    static_assert(std::is_same_v<sample_traits_t<lossless_traits<triplet<uint16_t>, 16>>, lossless_traits<uint16_t, 16>>);
    static_assert(std::is_same_v<sample_traits_t<lossless_traits<quad<uint16_t>, 16>>, lossless_traits<uint16_t, 16>>);

    // sample_traits_t: default_traits<S, S> is identity.
    static_assert(std::is_same_v<sample_traits_t<default_traits<uint8_t, uint8_t>>, default_traits<uint8_t, uint8_t>>);
    static_assert(std::is_same_v<sample_traits_t<default_traits<uint16_t, uint16_t>>, default_traits<uint16_t, uint16_t>>);

    // sample_traits_t: default_traits<S, PixelType> maps to default_traits<S, S>.
    static_assert(std::is_same_v<sample_traits_t<default_traits<uint8_t, pair<uint8_t>>>, default_traits<uint8_t, uint8_t>>);
    static_assert(
        std::is_same_v<sample_traits_t<default_traits<uint8_t, triplet<uint8_t>>>, default_traits<uint8_t, uint8_t>>);
    static_assert(std::is_same_v<sample_traits_t<default_traits<uint8_t, quad<uint8_t>>>, default_traits<uint8_t, uint8_t>>);
    static_assert(
        std::is_same_v<sample_traits_t<default_traits<uint16_t, pair<uint16_t>>>, default_traits<uint16_t, uint16_t>>);
    static_assert(
        std::is_same_v<sample_traits_t<default_traits<uint16_t, triplet<uint16_t>>>, default_traits<uint16_t, uint16_t>>);
    static_assert(
        std::is_same_v<sample_traits_t<default_traits<uint16_t, quad<uint16_t>>>, default_traits<uint16_t, uint16_t>>);

    // make_sample_traits: scalar lossless_traits returns the same instance (identity).
    {
        constexpr lossless_traits<uint8_t, 8> traits;
        [[maybe_unused]]
        const auto sample_traits{make_sample_traits(traits)};
        static_assert(std::is_same_v<decltype(sample_traits), const lossless_traits<uint8_t, 8>>);
    }

    // make_sample_traits: compound lossless_traits returns default-constructed scalar instance.
    {
        constexpr lossless_traits<triplet<uint8_t>, 8> traits;
        [[maybe_unused]]
        const auto sample_traits{make_sample_traits(traits)};
        static_assert(std::is_same_v<decltype(sample_traits), const lossless_traits<uint8_t, 8>>);
    }

    // make_sample_traits: default_traits with compound pixel type preserves runtime parameters.
    {
        const default_traits<uint8_t, triplet<uint8_t>> traits(255, 3);
        const auto sample_traits{make_sample_traits(traits)};
        static_assert(std::is_same_v<decltype(sample_traits), const default_traits<uint8_t, uint8_t>>);
        EXPECT_TRUE(sample_traits.maximum_sample_value == 255);
        EXPECT_TRUE(sample_traits.near_lossless == 3);
    }

    // make_sample_traits: default_traits<S, S> (already scalar) returns a copy.
    {
        const default_traits<uint16_t, uint16_t> traits(4095, 0);
        const auto sample_traits{make_sample_traits(traits)};
        static_assert(std::is_same_v<decltype(sample_traits), const default_traits<uint16_t, uint16_t>>);
        EXPECT_TRUE(sample_traits.maximum_sample_value == 4095);
        EXPECT_TRUE(sample_traits.near_lossless == 0);
    }
}

} // namespace charls::test
