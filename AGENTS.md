<!--
  SPDX-FileCopyrightText: © 2026 Team CharLS
  SPDX-License-Identifier: BSD-3-Clause
-->

# Repository Guidelines

CharLS is a portable C++17 implementation of JPEG-LS.
Keep changes aligned with its public API, cross-platform goals,
and existing behavior.

## Project layout

- `include/charls/`: public C and C++ APIs.
- `src/`: library implementation.
- `test/`: unit, documentation, and conformance tests.
- `cli/`, `samples/`, `benchmark/`, and `fuzzing/`: supporting tools and 
   projects.

## Changes and style

- Follow `.clang-format` and `.editorconfig`; C++ uses four spaces, no tabs, 
  and the C++17 baseline.
- Preserve public API and ABI compatibility unless the task explicitly calls 
  for a breaking change. Follow established error-handling and implementation 
  patterns.
- Add or update focused tests for behavior changes. Keep tests and 
  fixtures in `test/`.
- Follow neighboring files' SPDX headers and licensing conventions.
- See [doc/style_and_design.md](doc/style_and_design.md) for design rationale 
  and [CONTRIBUTING.md](.github/CONTRIBUTING.md) for contribution guidance.
- See also https://github.com/team-charls/charls/wiki/Building-CharLS build instructions.  

## Build and test

Configure with tests enabled, then build and run the test suite:

```sh
cmake -S . -B build -DCHARLS_BUILD_ALL=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

GoogleTest is found if installed, or fetched by CMake when needed. 
CMake presets are also available; use `cmake --list-presets` to see 
platform-specific options.
