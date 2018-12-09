# Style and Design

## Introduction

The purpose of this document is to record the basic style and design guidelines of CharLS to ensure consistency and 
to record decisions.

## Style

### Tabs and Spaces

Spaces with an indent of 4. This to ensure maximum readability with different editors/platforms.
To document this and ensure editors are configured correctly an .editorconfig is used in the root of the repository.

### Template: class vs typename

The typename keyword is the prefered keyword to "mark" template variables.

### Prevent header file double include

There are 2 methods to prevent double include:

1) #pragma once. Supported by all main compilers MSVC, GCC, clang and many other compilers, but in essence a compiler extension.

2) #ifdef CHARLS_<FILENAME> \ #define CHARLS_<FILENAME> \ #endif construction.

* Given the industry acceptance, use method 1 (easier and less manual code).

## Exceptions and Error Handling

* C++ Exceptions should be derived from std::exception. (Common accepted idom)
* The exception should be convertible to an error code to support the C API
* An english error text that describes the problem is extreme usefull.

=> Design: std::system_error is the standard solution to throw exceptions from libraries (CharLS is a library)

## Jpeg-LS Design decisions

### Width and Height

The Jpeg-LS standards support a height and weight up to 2^32-1. This means that at least an 32-bit unsigned integer is
needed to support the complete range. Using unsigned integers has however the drawback that the interoperability with other languages is poort:

C# : supports unsigned 32 bit integers (but .NET marks them as not CLS compliant)
VB.NET : supports unsigned 32 bit integers (but .NET marks them as not CLS compliant)
Java : by default integers are signed
Javascript: only signed integers
Python: only signed integers

Given the practical applications that 2^31 * 2^31 (max signed integer) will be sufficient for the coming 10 years, the API should use signed integers.
References: 
8K Images = (7680×4320)

### Supported C++ language

CharLS currently targets C++14 on the main branch. This will be done until December 2020 (3 years after the release of C++17)

#### Features currently not available (C++17)

* nodiscard attribute
* maybe_unused attribute
* Inline variables
* Guaranteed copy elision
* constexpr if-statements
* __has_include
* std::byte
* clamp ?

#### Features currently not available (C++20)

The following features are available in C++20 (usable after 2023), or in dual language support mode.

* endian
* <span>
* modules
