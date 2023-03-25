// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

// The macros below are needed to export/import the Application Binary Interface of the charls library.

#if defined(CHARLS_STATIC)

// CharLS is used as a static library, just define the entry points as extern.
#define CHARLS_API_IMPORT_EXPORT extern
#define CHARLS_API_CALLING_CONVENTION

#else

// CharLS is used as a DLL \ shared library, define the entry points as required.
#if defined(_WIN32)

#if defined(CHARLS_LIBRARY_BUILD)
#define CHARLS_API_IMPORT_EXPORT __declspec(dllexport)
#else
#define CHARLS_API_IMPORT_EXPORT __declspec(dllimport)
#endif

// Ensure that the exported functions of a 32 bit Windows DLL use the __stdcall convention.
#if defined(_M_IX86) || defined(__MINGW32__)
#define CHARLS_API_CALLING_CONVENTION __stdcall
#else
#define CHARLS_API_CALLING_CONVENTION
#endif

#else

#if defined(CHARLS_LIBRARY_BUILD)
#define CHARLS_API_IMPORT_EXPORT __attribute__((visibility("default")))
#else
#define CHARLS_API_IMPORT_EXPORT extern
#endif

#define CHARLS_API_CALLING_CONVENTION

#endif

#endif


#ifdef __cplusplus

#define CHARLS_FINAL final
#define CHARLS_NOEXCEPT noexcept
#define CHARLS_C_VOID

#else

#define CHARLS_FINAL
#define CHARLS_NOEXCEPT
#define CHARLS_C_VOID void

#endif
