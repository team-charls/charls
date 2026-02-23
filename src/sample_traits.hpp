// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "default_traits.hpp"
#include "lossless_traits.hpp"

namespace charls {

/// <summary>
/// Extracts the sample type from a pixel type.
/// For scalar types this is the identity; for compound types (pair/triplet/quad) it extracts the element type.
/// </summary>
template<typename T>
struct extract_sample
{
    using type = T;
};

template<typename T>
struct extract_sample<pair<T>>
{
    using type = T;
};

template<typename T>
struct extract_sample<triplet<T>>
{
    using type = T;
};

template<typename T>
struct extract_sample<quad<T>>
{
    using type = T;
};

/// <summary>
/// Type mapping from full Traits (which include pixel_type) to sample-level Traits.
/// This allows heavy codec functions to be instantiated only once per unique sample-level Traits,
/// rather than once per pixel type (pair/triplet/quad variants share the same sample-level code).
/// </summary>

// Primary: identity mapping (covers scalar lossless_traits and default_traits<S, S>)
template<typename Traits>
struct sample_traits_of
{
    using type = Traits;
};

// default_traits<SampleType, PixelType> -> default_traits<SampleType, SampleType>
template<typename SampleType, typename PixelType>
struct sample_traits_of<default_traits<SampleType, PixelType>>
{
    using type = default_traits<SampleType, SampleType>;
};

// lossless_traits<PixelType, B> -> lossless_traits<SampleType, B>
// For scalar PixelType this is identity; for compound types it extracts the sample type.
template<typename PixelType, int32_t BitsPerSample>
struct sample_traits_of<lossless_traits<PixelType, BitsPerSample>>
{
    using type = lossless_traits<typename extract_sample<PixelType>::type, BitsPerSample>;
};

template<typename Traits>
using sample_traits_t = typename sample_traits_of<Traits>::type;


/// <summary>
/// Constructs a SampleTraits instance from a full Traits instance.
/// For lossless_traits (stateless), returns a default-constructed instance.
/// For default_traits, constructs with the same parameters (sample_type as pixel_type).
/// </summary>
template<typename Traits>
[[nodiscard]]
auto make_sample_traits(const Traits& traits)
{
    using sample_traits_type = sample_traits_t<Traits>;

    if constexpr (std::is_same_v<Traits, sample_traits_type>)
    {
        return traits;
    }
    else if constexpr (sample_traits_type::always_lossless)
    {
        return sample_traits_type{};
    }
    else
    {
        return sample_traits_type{traits.maximum_sample_value, traits.near_lossless};
    }
}

} // namespace charls
