# SPDX-FileCopyrightText: © 2025 Team CharLS
# SPDX-License-Identifier: BSD-3-Clause

"""
Defines the copts flags for the supported compilers and select the matching options
"""

CHARLS_MSVC_FLAGS = [
    "/W3",
    "/wd4005",
    "/wd4068",
    "/wd4180",
    "/wd4503",
    "/wd4800",
    "/D_CRT_SECURE_NO_WARNINGS",
    "/D_SCL_SECURE_NO_WARNINGS",
    "/D_ENABLE_EXTENDED_ALIGNED_STORAGE",
    "/std:c++17",
    "/permissive-",
]

CHARLS_GCC_FLAGS = [
    "-Wall",
    "-Wextra",
    "-Wcast-qual",
    "-Wformat=2",
    "-Wmissing-declarations",
    "-Woverlength-strings",
    "-Wpointer-arith",
    "-Wundef",
    "-Wunused-local-typedefs",
    "-Wunused-result",
    "-Wvarargs",
    "-Wvla",
    "-Wwrite-strings",
]

CHARLS_LLVM_FLAGS = [
    "-Wall",
    "-Wextra",
    "-Wcast-qual",
    "-Wconversion",
    "-Wdeprecated-pragma",
    "-Wfloat-overflow-conversion",
    "-Wfloat-zero-conversion",
    "-Wfor-loop-analysis",
    "-Wformat-security",
    "-Wgnu-redeclared-enum",
    "-Winfinite-recursion",
    "-Winvalid-constexpr",
    "-Wliteral-conversion",
    "-Wmissing-declarations",
    "-Wnullability-completeness",
    "-Woverlength-strings",
    "-Wpointer-arith",
    "-Wself-assign",
    "-Wshadow-all",
    "-Wshorten-64-to-32",
    "-Wno-sign-conversion",
    "-Wstring-conversion",
    "-Wtautological-overlap-compare",
    "-Wtautological-unsigned-zero-compare",
    "-Wthread-safety",
    "-Wundef",
    "-Wuninitialized",
    "-Wunreachable-code",
    "-Wunused-comparison",
    "-Wunused-local-typedefs",
    "-Wunused-result",
    "-Wvla",
    "-Wwrite-strings",
]

CHARLS_DEFAULT_COPTS = select({
    "@rules_cc//cc/compiler:msvc-cl": CHARLS_MSVC_FLAGS,
    "@rules_cc//cc/compiler:clang": CHARLS_LLVM_FLAGS,
    "@rules_cc//cc/compiler:gcc": CHARLS_GCC_FLAGS,
    "//conditions:default": [],
})
