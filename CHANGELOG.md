# Change Log

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/) and this project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased]

### Changed

- Improved the validation of the JPEG stream during decoding
- #pragma once is now used to prevent that header files are included multiple times (supported by all modern C++ compilers)
- The referenced NuGet packages of the .NET wrapper assembly are updated to their latest versions
- The CMake build script has been updated to modern CMake and requires at least CMake 3.9
- charls_error has been replaced by a C++11 compatible jpegls_errc error code enum design
- All types are now in the charls C++ namespace
- Support for .NET Code Contracts has been removed as this technology is being phased out by Microsoft

### Fixed

- Fixes [#35](https://github.com/team-charls/charls/issues/35), Encoding will fail if the bit per sample is greater than 8, and a custom RESET value is used
- Fixes [#51](https://github.com/team-charls/charls/issues/51), The default threshold values are not corrected computed for 6 bit images or less
- Fixed the ASSERT in ModuloRange, which would trigger false assertions in debug builds

## [2.0.0] - 2016-5-18

### Changed

- Updated the source code to C++14
- Refactored several APIs to make usage of the library easier

### Fixed

- Fixes [#10](https://github.com/team-charls/charls/issues/10), Fixed the problem that "output buffer to small" was not detected when writting encoded bytes to a fixed output buffer. This could cause memory corruption problems
- Fixes [11](https://github.com/team-charls/charls/issues/11), Update charlstest to return EXIT_SUCCESS/FAILURE
- Fixed the issue that DecodeToPnm would set params.colorTransform = ColorTransformation::BigEndian but the library didn’t support this option during decoding

## [1.1.0] - 2016-5-15

### Fixed

- Fixes [#9](https://github.com/team-charls/charls/issues/9) EncoderStrategy::AppendToBitStream method fails if buffer is full, 31 bits are added and xFF bytes are written

## [1.0.0] - 2010-11-18

First release of the CharLS JPEG-LS codec.
