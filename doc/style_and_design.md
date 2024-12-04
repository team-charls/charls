<!--
  SPDX-FileCopyrightText: © 2014 Team CharLS
  SPDX-License-Identifier: BSD-3-Clause
-->

# Style and Design

## Introduction

The purpose of this document is to record the basic style and design guidelines of CharLS to ensure consistency and
to record decisions.

## Style

### Tabs and Spaces

Spaces with an indent of 4. This to ensure maximum readability with different editors/platforms.
To document this and ensure editors are configured correctly an .editorconfig file and a .clang-format file
is placed in the root of the repository.

### Template: class vs typename

The typename keyword is the preferred keyword to "mark" template variables.

### Prevent header file double include

There are 2 methods to prevent double include:

1) #pragma once. Supported by all main compilers MSVC, GCC, clang and many other compilers, but in essence a compiler extension.

2) #ifdef CHARLS_\<FILENAME> \ #define CHARLS_\<FILENAME> \ #endif construction.

* Given the industry acceptance, use method 1 (easier and less manual code).

## Exceptions and Error Handling

* C++ exceptions should be derived from `std::exception`. (Common accepted idiom)
* The exception should be convertible to an error code to support the C API
* An english error text that describes the problem is extreme useful.

=> Design: std::system_error is the standard solution to throw exceptions from libraries (CharLS is a library)

By deriving an exception type from `std::system_error` user code can catch CharLS exceptions by type.
It make sense to support this common pattern. The alternative is

``` cpp
catch (const std::system_error& e)
{
    if (e.code.category() == charls::jpegls_category())
    {
        // Handle JPEG-LS errors.
    }

    // Note that checking for a single error can be done directly.
    if (e.code == charls::jpegls_invalid_argument_width)
    {
        // ...
    }
}
```

## Types

The type charls_jpegls_encoder is placed in the charls namespace as the type needs also to be defined as a C type.
It makes it also easier to keep the header only type charls::jpegls_encoder and the implementation class charls_jpegls_encoder separate.

## Jpeg-LS Design decisions

### Width and Height

The Jpeg-LS standards support a height and weight up to 2^32-1. This means that at least an 32-bit unsigned integer is
needed to support the complete range. Using unsigned integers has however the drawback that the interoperability with other languages is poor:

C# : supports unsigned 32 bit integers (but .NET marks them as not CLS compliant)
VB.NET : supports unsigned 32 bit integers (but .NET marks them as not CLS compliant)
Java : by default integers are signed
Javascript: only signed integers
Python: only signed integers

Having unsigned integers in the C and C++ application and signed integers in wrapping libraries should
not be a practical problem. Most real world images can be expressed in signed integers,  8K Images = (7680×4320).

### ABI

A C style interface is used to ensure that the ABI is stable. However just a C style API is not enough.
As CharLS can be distributes as a dynamic link library also the filename needs to be managed.

* It should be possible for applications to use v1 and v2 DLL at the same time.

* It should be possible to load the correct CPU architecture from the same directory.

Design (Windows):

* filename = charls-\<ABI version>-\<CPU architecture>.dll

#### Null pointer checking

Passing a NULL pointer as parameter into the C ABI can be handled in 2 ways:

* There can be an explicit check and an error return value.

* The pointer can be dereferenced directly and passing NULL will just crash the process.

Passing a NULL pointer is a defect of the calling application, it is however helpful to
not generate an access violation inside the library module. If the library is build without
symbol info, it is difficult for the user to detect this mistake. Returning a "bad parameter"
error is in this case more helpful.
Note: NULL is the only special value that can be checked, but also the common mistake.

### Variable names versus JPEG-LS Standard

The JPEG-LS standard uses pseudo-code to define certain parts of the algorithm. It makes
sense to define a good naming convention. Not all JPEG-LS names are good C++ variable\parameter names.

| JPEG-LS Symbol | C++ name                 | Description |
| -------------- | ------------------------ |------------ |
| a, b, c, d     | a, b, c, d               | positions of samples in the causal template |
| bpp            | bits_per_sample          | number of bits needed to represent MAXVAL (not less than 2) |
| D1, D2, D3, Di | d1, d2, d3, di           | local gradients |
| EMErrval       | e_mapped_error_value     | Errval mapped to non-negative integers in run interruption mode |
| Errval         | error_value              | prediction error (quantized or unquantized, before and after modulo reduction) |
| ILV            | interleave_mode          | indication of the interleave mode used for the scan |
| LIMIT          | limit                    | the value of glimit for a sample encoded in regular mode |
| J[0..31]       | J[0..31]                 | 32 variables indicating order of run-length codes |
| k              | k (or golomb_code)       | Golomb coding variable for regular mode |
| MErrval        | mapped_error_value       | Errval mapped to non-negative integers in regular mode |
| MAXVAL         | maximum_sample_value     | maximum possible image sample value over all components of a scan |
| NEAR           | near_lossless            | difference bound for near-lossless coding |
| Px             | predicted_value          | predicted value for the current sample |
| Q1, Q2, Q3, Qi | q1, q2, q3, qi           | region numbers to quantize local gradients |
| qbpp           | quantized_bits_per_pixel | number of bits needed to represent a mapped error value |
| Ra, Rb, Rc, Rd | ra, rb, rc, rd           | reconstructed values of samples in the causal template |
| RANGE          | range                    | range of prediction error representation
| RESET          | reset_threshold          | threshold value at which A, B, and N are halved |

### Supported C++ language

CharLS currently targets C++17 on the main branch. Upgrading to C++20 will be done when C++20 is used in
mainstream C++ development.

#### Features currently not available (C++20)

The following features are available in C++20, or in dual language support mode.

* endian
* \<span>
* modules
* [[likely]] and [[unlikely]]
* std::countl_zero

#### Features currently not available (C++23)

The following features are available in C++23, or in dual language support mode.

* std::byteswap
* import std;
* std::to_underlying
* std::unreachable

### Portable Anymap Format

The de facto standard used by the JPEG standard to deliver test files is the Portable Anymap Format.
This format has been made popular by the netpbm project. It is an extreme simple format and only
designed to make it easy to exchange images on many platforms.
It consists of the following variants

* P5 = Portable Graymap (0 ... 16 bits monochrome), extension = .pgm
* P6 = Portable PixMap (0 .. 16 bits RGB), extension = .ppm
* P7 = Portable Arbitrary Map (0..16 bits, N channels), extension = .pam

### External components \ Package Manager

One of the missing features of C++ is a standard Package Manager. The following packages would be useful to use:

* Cross-platform unit test library (for example Catch2)
* Library to read Anymap files (for example Netpbm)
* Library to parse command line parameters (for example Clara, CLI11)

### Performance

#### Decoding the bitstream

There are 2 main ways to decode the bitstream

* Basic: Read it byte for byte and store the results in a cache variable

* Improved: read when possible a register in 1 step. The problem is that 0xFF can exists in the
bitstream. If such a 0xFF exists the next bit needs to be ignored. There are a couple of way to do this:
  * A) Search for the first position with a 0xFF it and remember the position. 0xFFs are rare.
  * B) Search for the first 0xFF with memchr. memchr can leverage special CPU instructions when possible.
  * C) Load a register and check if it contains a 0xFF byte.

Measurements conclusion: option B is the fastest on x64. This is the original algorithm. There is not a large difference between the different options.
Examples of decoding performance on a AMD 5950X x64 CPU:

| Image                       | Basic   | Improved A | Improved B |Improved C |
| --------------              | ------- | ---------- | ---------- |---------- |
| 16 bit 512 * 512 (CT image) | 3.09 ms | 3.17 ms    | 3.06 ms    | 3.10 ms   |
|  8 bit 5412 * 7216          | 517 ms  | 509 ms     | 507 ms     | 512 ms    |

#### Using special "checked" functions for decoding

The input data for decoding is untrusted. Additional checks are included to detect invalid bitstream data.
Most of these routines are also shared with the encoding process.
Measurements show that there is no impact by creating special functions that don't these checks.
In normal cases the checks never fail and the unlikely branch is never taken. This makes is very easy for
the CPU branch predictor to always the correct outcome based on the branch history.

### Supported C++ Compilers

#### Clang

Recommended warnings:

* -Wall (warning collection switch)
* -Wextra (warning collection switch)
* -Wnon-gcc (warning collection switch)
* -Walloca (not included in Wall or Wextra)
* -Wcast-qual (not included in Wall or Wextra)
* -Wformat=2 (not included in Wall or Wextra)
* -Wformat-security (enabled by -Wformat=2)
* -Wnull-dereference (enabled by default)
* -Wstack-protector (enabled by default)
* -Wvla (not included in Wall or Wextra)
* -Warray-bounds (enabled by default)
* -Warray-bounds-pointer-arithmetic (not included in Wall or Wextra)
* -Wassign-enum (not included in Wall or Wextra)
* -Wbad-function-cast (not included in Wall or Wextra)
* -Wconditional-uninitialized (not included in Wall or Wextra)
* -Wconversion (enabled by Wnon-gcc)
* -Widiomatic-parentheses (not included in Wall or Wextra)
* -Wimplicit-fallthrough (not included in Wall or Wextra)
* -Wloop-analysis (not included in Wall or Wextra)
* -Wpointer-arith (not included in Wall or Wextra)
* -Wshift-sign-overflow (not included in Wall or Wextra)
* -Wshorten-64-to-32 (enabled by Wnon-gcc)
* -Wswitch-enum (not included in Wall or Wextra)
* -Wtautological-constant-in-range-compare (not included in Wall or Wextra)
* -Wunreachable-code-aggressive (not included in Wall or Wextra)
* -Wthread-safety (not included in Wall or Wextra)
* -Wthread-safety-beta (not included in Wall or Wextra)
* -Wcomma (not included in Wall or Wextra)
