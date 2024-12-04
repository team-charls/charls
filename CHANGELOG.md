<!--
  SPDX-FileCopyrightText: © 2017 Team CharLS
  SPDX-License-Identifier: BSD-3-Clause
-->

# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project adheres to [Semantic Versioning](http://semver.org/).

## [3.0.0] - UNRELEASED

### Added

- Support to encode and decode mapping tables.
- Support to retrieve the height from a DNL marker segment.
- Support to encode and decode mixed interleaved mode scans.

### Fixed

- Endless loop when decoding invalid JPEG-LS input data.

### Changed

- BREAKING: Updated the minimal required C++ language version to C++17.
- BREAKING: encoding_options::include_pc_parameters_jai is not enabled by default anymore.
- BREAKING: charls::jpegls_decoder and charls::jpegls_encoder follow the same const pattern as the C API.
- BREAKING: the failure values of the enum charls::jpegls_errc are now divided in 2 groups: runtime errors and logic errors.
- BREAKING: The public charls.h header has been split into charls.h (C applications) and charls.hpp (C++ applications).
- BREAKING: Method charls_jpegls_decoder_get_interleave_mode has an additional extra parameter: component_index.

### Removed

- BREAKING: Deprecated legacy 1.x API methods have been removed.

## [2.4.2] - 2023-5-16

### Fixed

- Fixed [#269](https://github.com/team-charls/charls/issues/269), Decoding doesn't work when compiled with mingw64.

## [2.4.1] - 2023-1-2

### Fixed

- Fixed [#221](https://github.com/team-charls/charls/issues/221), jpegls_errc::destination_too_small incorrectly thrown for 8 bit 2*2 image with stride = 4 during decoding.

## [2.4.0] - 2022-12-29

### Added

- Support for Windows on ARM64 in the MSBuild projects and CMake files.
- Support to read and write application data markers. [#180](https://github.com/team-charls/charls/issues/180)
- Added method charls_validate_spiff_header to validate SPIFF headers.

### Changed

- Improved compatibility of public headers with C++20.
- Switch order of APP8 and SOF55 markers during encoding to align with user application data markers.

### Fixed

- Fixed [#167](https://github.com/team-charls/charls/issues/196), Multi component image with interleave mode none is not correctly decoded when a custom stride argument is used.

## [2.3.4] - 2022-2-12

### Changed

- Replaced legacy test images.

## [2.3.3] - 2022-2-5

### Changed

- CTest is now used in the CI build pipeline to test the output of the Linux and macOS builds.

### Fixed

- Fixed [#167](https://github.com/team-charls/charls/issues/167), Decoding\Encoding fails on IBM s390x CPU (Big Endian architecture).

## [2.3.2] - 2022-1-29

### Changed

- Updates to the CMakeLists.txt for Unix builds (except macOS) to hide more symbols from the shared library.
- C\++14 is now the minimum version instead of explicitly required. This allows consuming applications more flexibility.
Typically CMake will select the latest C++ standard version that the used C++ compiler supports.

### Fixed

- Fixed [#160](https://github.com/team-charls/charls/issues/160), warning: cast from ‘unsigned char*’ to ‘uint16_t*’ increases required alignment of target type.
- Fixed [#161](https://github.com/team-charls/charls/issues/161), warning: useless cast to type ‘size_t’ {aka ‘unsigned int’} [-Wuseless-cast].

## [2.3.1] - 2022-1-25

### Fixed

- Fixed [#155](https://github.com/team-charls/charls/issues/155), charls::jpegls_decoder::decode: 2 overloads have similar conversions in v2.3.0

## [2.3.0] - 2022-1-24

### Added

- The encoder API has been extended with a rewind method that can be used to reuse a configured encoder to encode multiple images in a loop.
- Added support to decode JPEG-LS images that use restart markers [#92](https://github.com/team-charls/charls/issues/92).
- Added support to write and read comment (COM) segments [#113](https://github.com/team-charls/charls/issues/113).
- Added support to encode/decode oversized images (width or height larger then 65535).
- Extended the validation of the encoded JPEG-LS byte stream during decoding.
- Added support to encode JPEG-LS images with:
  - The option to ensure the output stream has an even size.
  - The option to write the CharLS version number as a comment (COM segment) to the output stream.
  - The option to write the coding parameters to the output stream if the bits per pixel are larger then 12 (enabled by default).
- Usage of compiler specific attributes on the public API as replacement for ``[[nodiscard]]`` (which is a C++17 feature).

### Changed

- CMakeSettings.json has been replaced with CMakePresets.json.
- Non default coding parameters are explicitly stored in the output stream during encoding.
- GCC shared library release builds are now using LTO (Link Time Optimization).
- Some functions use compiler intrinsics for slightly better performance.

### Fixed

- Fixed [#84](https://github.com/team-charls/charls/issues/84), Default preset coding parameters not computed for unset values.
- Fixed [#102](https://github.com/team-charls/charls/issues/102), CMake find_package(charls 2.2.0 REQUIRED) not working.

## [2.2.1] - 2022-2-3

### Fixed

- Backport of fix for [#167](https://github.com/team-charls/charls/issues/167), Decoding\Encoding fails on IBM s390x CPU (Big Endian architecture).

## [2.2.0] - 2021-1-10

### Added

- Added pkg-config charls.pc file to help in detect the CharLS library (see [#76](https://github.com/team-charls/charls/issues/76))
- Added standard CMake variable BUILD_SHARED_LIBS as an option to make it visible in the CMake GUI (see [#66](https://github.com/team-charls/charls/issues/66))
- The PowerPC Little Endian (ppc64le) platform has been added as supported architecture

### Changed

- The API has been extended with additional annotations to assist the static analyzer in the MSVC and GCC/clang compilers
- The size check for a Start Of Scan (SOS) segment is now exact for improved compatibility with fuzzy testing
- The minimum support version of CMake is now 3.13 (was 3.9), 3.13 is needed for add_link_options
- The Windows static library and DLL are now compiled with the Control Flow Guard (/guard:cf) option enabled for enhanced security
- The .NET adapter has been upgraded to .NET 5 and moved to its own [repository](https://github.com/team-charls/charls-native-dotnet)
This has been done to make it possible to have different release cycles.

### Removed

- The legacy methods JpegLsEncodeStream, JpegLsDecodeStream and JpegLsReadHeaderStream have been removed as exported methods.
  These methods were not part of the public API and only used by by the charlstest application

### Fixed

- Fixed [#21](https://github.com/team-charls/charls/issues/21), Building with UBSAN, will report runtime error: left shift
  of 4031 by 63 places cannot be represented in type 'long int'
- Fixed [#25](https://github.com/team-charls/charls/issues/25), CharLS fails to read LSE marker segment after first SOS segment
- Fixed [#26](https://github.com/team-charls/charls/issues/26), CharLS should only use the valid bits from the passed input buffer
- Fixed [#36](https://github.com/team-charls/charls/issues/36), CharLS should remain stable from bad input (several issues found by fuzzy testing)
- Fixed [#60](https://github.com/team-charls/charls/issues/60), Visual Studio 2015 C++ compiler cannot compile certain constexpr constructions
- Fixed [#62](https://github.com/team-charls/charls/issues/62), Missing includes in jpegls_error.cpp when using libc++ (and not libstdc++)
- Fixed [#70](https://github.com/team-charls/charls/issues/70), The C and C++ sample don't swap the pixels from a .bmp file horizontal
- Fixed [#79](https://github.com/team-charls/charls/issues/79), Wrong JPEG-LS encoding when stride is non-default (stride != 0),
  component count > 1 and interleave_mode is none

## [2.1.0] - 2019-12-29

### Added

- Two new C++ classes (jpegls_encoder \ jpegls_decoder) have been added to make it much easier to use CharLS from C++
- A new C API (charls_xxx functions) was added to provide a more stable ABI for future updates. The old API calls are internally forwarded to the new API
- CharLS can now read and write JPEG-LS standard SPIFF headers
- Support has been added to detect the unsupported JPEG-LS extension (ISO/IEC 14495-2) SOF_57 marker and IDs in LSE marker
- The unit test project has been extended and now includes 188 tests
- Support has been added to encode\decode 4 component images in all interleave modes

### Changed

- charls_error has been replaced by a C++11 compatible jpegls_errc error code enum design
- The included C and C++ sample have been updated to use the new C\C++ API
- Improved the validation of the JPEG-LS stream during decoding
- #pragma once is now used to prevent that header files are included multiple times (supported by all modern C++ compilers)
- The referenced NuGet packages of the .NET wrapper assembly are updated to their latest versions
- The CMake build script has been updated to modern CMake and requires at least CMake 3.9
- All types are now in the charls C++ namespace
- All source code files now use the SPDX Unique License Identifiers (BSD-3-Clause) to identify the license

### Deprecated

- The legacy 1.x\2.0 C API has been marked as deprecated. This legacy API will be maintained until the next major upgrade
  Future 2.x updates will start to mark the legacy types and functions with the C++ ```[[deprecated]]``` attribute

### Removed

- Support to write JFIF headers during encoding has been removed. JFIF headers were already skipped when present during decoding.
  SPIFF headers should be used when creating standalone .jls files
- Support for .NET Code Contracts has been removed as this technology is being phased out by Microsoft

### Fixed

- Fixed [#7](https://github.com/team-charls/charls/issues/7), How to compile CharLS with Xcode has been documented in the Wiki
- Fixes [#35](https://github.com/team-charls/charls/issues/35), Encoding will fail if the bit per sample is greater than 8, and a custom RESET value is used
- Fixed [#44](https://github.com/team-charls/charls/issues/44), Only the API functions should be exported from a Linux shared library
- Fixes [#51](https://github.com/team-charls/charls/issues/51), The default threshold values are not corrected computed for 6 bit images or less
- Fixed the ASSERT in the ModuloRange function, which would trigger false assertions in debug builds

## [2.0.0] - 2016-5-18

### Changed

- Updated the source code to C++14
- Refactored several APIs to make usage of the library easier

### Fixed

- Fixes [#10](https://github.com/team-charls/charls/issues/10), Fixed the problem that "output buffer to small" was not
  detected when writing encoded bytes to a fixed output buffer. This could cause memory corruption problems
- Fixes [11](https://github.com/team-charls/charls/issues/11), Update charlstest to return EXIT_SUCCESS/FAILURE
- Fixed the issue that DecodeToPnm would set params.colorTransform = ColorTransformation::BigEndian but the library didn’t support this option during decoding

## [1.1.0] - 2016-5-15

### Fixed

- Fixes [#9](https://github.com/team-charls/charls/issues/9) EncoderStrategy::AppendToBitStream method fails if buffer is full, 31 bits are added and xFF bytes are written

## [1.0.0] - 2010-11-18

First release of the CharLS JPEG-LS codec.
