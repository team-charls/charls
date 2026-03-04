<!--
  SPDX-FileCopyrightText: © 2026 Team CharLS
  SPDX-License-Identifier: BSD-3-Clause
-->

# Instructions to fuzz CharLS with LibFuzzerEncoder

- It is in general recommended to fuzz the release builds.
  The release build runs faster and more fuzzing can be done in a time period.

## Windows (MSbuild projects)

- Enable the project LibFuzzerEncoder in the Visual Studio Configuration Manager  
  It is excluded by default as Visual Studio 2019 cannot build this project.
- Update the release configuration of the CharLS MSbuild project and enable address sanitizer.
- Build the solution with Visual Studio 2022 17.8 or newer.
- Run the LibFuzzerTest from the command line (-help=1) will show the options.

## Linux\Windows (CMake)

Remark: Using LibFuzzer requires Clang or Visual Studio 2022

- Enable the address sanitizer CMake option (CHARLS_ENABLE_ASAN)
- Build the targets (RelWithDebInfo)
- Run the LibFuzzerTest from the command line (-help=1) will show the options.

## Running the Fuzzer

### Basic Usage

```bash
# Run with default settings
./fuzzer-encoder

# Run with a specific max input size
./fuzzer-encoder -max_len=100000

# Run with existing corpus
./fuzzer-encoder corpus/
```

## Corpus Management

A corpus directory can be created to store interesting inputs:

```bash
mkdir corpus
./fuzzer-encoder corpus/ -max_len=100000
```

Over time, libfuzzer will expand the corpus with new inputs that increase code coverage.

## Getting coverage information

To analyze which parts of CharLS have been reached by the fuzzer it is possible to re-run the corpus files.
The procedure is:

- Run the application with the fuzzer with the command line option to indicate the corpus directory
- Rename main_coverage into main()
- Rebuild the application and run Microsoft.CodeCoverage.Console collect LibFuzzerEncoder.exe
- Open the generated coverage file into visual studio.
