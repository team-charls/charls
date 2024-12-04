<!--
  SPDX-FileCopyrightText: © 2023 Team CharLS
  SPDX-License-Identifier: BSD-3-Clause
-->

# Instructions to fuzz CharLS with LibFuzzer

- It is in general recommended to fuzz the release builds.
  The release build runs faster and more fuzzing can be done in a time period.

## Windows (MSbuild projects)

- Enable the project LibFuzzerTest in the Visual Studio Configuration Manager  
  It is excluded by default as Visual Studio 2019 cannot build this project.
- Update the release configuration of the CharLS MSbuild project and enable address sanitizer.
- Build the solution with Visual Studio 2022 17.8 or newer.
- Run the LibFuzzerTest from the command line (-help=1) will show the options.

## Linux\Windows (CMake)

Remark: Using LibFuzzer requires Clang or Visual Studio 2022

- Enable the address sanitizer CMake option (CHARLS_ENABLE_ASAN)
- Build the targets (RelWithDebInfo)
- Run the LibFuzzerTest from the command line (-help=1) will show the options.
