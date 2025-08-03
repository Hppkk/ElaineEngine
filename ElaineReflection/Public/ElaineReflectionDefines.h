#pragma once
#define USE_REFLECTION 1 // Enable reflection features

#if USE_REFLECTION
#define REFLECTION_META(...) __attribute__((annotate("reflection_meta";#__VA_ARGS__)))
#else
#define REFLECTION_META(...)
#endif