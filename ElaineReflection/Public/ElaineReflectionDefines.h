#pragma once

// ============================================================
// Elaine Reflection Macros
// These macros are no-ops for the compiler. They serve as semantic
// markers parsed by ElaineBuildTool to generate reflection metadata.
//
// The .generated.h file will #undef and #define GENERATED_BODY()
// to expand to StaticClass(), GetClass(), StaticClassName().
// ============================================================

// Class marker: ECLASS() class MyClass { GENERATED_BODY() ... };
#define ECLASS(...)

// Property marker: EPROPERTY() float Speed;
#define EPROPERTY(...)

// Function marker: EFUNCTION() void Fire();
#define EFUNCTION(...)

// Default GENERATED_BODY — expands to nothing.
// The .generated.h for each class #undef's and redefines this macro
// with the appropriate StaticClass/GetClass/StaticClassName declarations.
#ifndef GENERATED_BODY
#define GENERATED_BODY()
#endif