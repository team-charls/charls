[![Build status](https://ci.appveyor.com/api/projects/status/yq0naf3v2m8nfa8r/branch/1.x-master?svg=true)](https://ci.appveyor.com/project/vbaderks/charls/branch/1.x-master)
[![Build Status](https://travis-ci.org/team-charls/charls.svg?branch=1.x-master)](https://travis-ci.org/team-charls/charls)

# CharLS 1.x, a JPEG-LS library

## Project Description

An optimized implementation of the JPEG-LS standard for lossless 
and near-lossless image compression. JPEG-LS is a low-complexity
standard that matches JPEG 2000 compression ratios.
In terms of speed, CharLS outperforms open source and 
commercial JPEG LS implementations.

IMPORTANT: this is the 1.x branch, which is C++03 compatible and in maintenaince mode to support platforms on which a C++14 compiler is not available. The general recommendation is to use the releases from the master branch.

### Features

* High performance C++03 implementation with a C interface to ensure maximum interopability.
* Supports Windows, Linux and Solaris in 32 bit and 64 bit.

## About JPEG-LS

JPEG-LS (ISO-14495-1:1999/ITU-T.87) is a standard derived from the Hewlett Packard LOCO algorithm. JPEG LS has low complexity (meaning fast compression) and high compression ratios, similar to JPEG 2000. JPEG-LS is more similar to the old Lossless JPEG than to JPEG 2000, but interestingly the two different techniques result in vastly different performance characteristics. 
[Wikipedia on lossless JPEG and JPEG-LS](en.wikipedia.org/wiki/Lossless_JPEG)
[Benchmarks JPEG-LS-Performance](charls.codeplex.com/wikipage?title=JPEG-LS-Performance&referringTitle=Home)

## Building CHARLS

### Primary development

Primary development is done on Windows with Visual Studio 2017.
Optimized projects and solution files are available for this environment.

All other platforms are supported by cmake. CMake is a cross platform
open source build system that generate project\make files for a large
set of environments.
