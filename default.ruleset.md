# Comments on disabled Visual Studio C++ Core Guidelines Rules

C26426: Global initializer calls a non-constexpr function 'xxx'
-> Rationale: many false warnings. CharLS is a library, globals are correctly initialized.

C26429: Symbol 'xxx' is never tested for nullness, it can be marked as not_null (f.23).
-> Rationale: Prefast attributes are better.

C26446: Prefer to use gsl::at() instead of unchecked subscript operator.
 -> Rationale: CharLS require good performance, gsl:at() cannot be used. debug STL already checks.

C26459: You called an STL function '' with a raw pointer parameter. Consider wrapping your range in a gsl::span and pass as a span iterator (stl.1)
Rationale: gsl:span() cannot be used. Update to std:span when available (C++20).

C26472: Don't use static_cast for arithmetic conversions
 -> Rationale: can only be solved with gsl::narrow_cast

C26481: Do not pass an array as a single pointer.
-> Rationale: gsl::span is not available.

C26482: Only index into arrays using constant expressions.
-> Rationale: static analysis can verify access, std::array during runtime (debug)

C26490: Don't use reinterpret_cast
-> Rationale: required to cast unsigned char* to char*.

C26494: Variable 'x' is uninitialized. Always initialize an object
-> Rationale: many false warnings, other analyzers are better.
