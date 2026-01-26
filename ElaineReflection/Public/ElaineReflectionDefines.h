#pragma once
#define USE_REFLECTION 1 // Enable reflection features

#if USE_REFLECTION
#define REFLECTION_META(...) __attribute__((annotate("reflection_meta;"#__VA_ARGS__)))
#define CLASS(...) class __attribute__((annotate("reflection_class;"#__VA_ARGS__)))
#define PROPERTY(...) __attribute__((annotate("reflection_property;"#__VA_ARGS__)))
#define FUNCTION(...) __attribute__((annotate("reflection_function;"#__VA_ARGS__)))
#define UNION(...) union __attribute__((annotate("reflection_union;"#__VA_ARGS__)))
#else
#define REFLECTION_META(...)
#define CLASS(...) class
#define PROPERTY(...)
#define FUNCTION(...)
#define UNION(...) union
#endif