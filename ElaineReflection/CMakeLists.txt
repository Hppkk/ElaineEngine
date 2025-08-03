
file(GLOB_RECURSE publicFiles Public)
source_group("Public" FILES ${publicFiles})
file(GLOB_RECURSE privateFiles Private)
source_group("Private" FILES ${privateFiles})


file(GLOB_RECURSE DIR_SRCS *.cpp)
file(GLOB_RECURSE DIR_INC *.h)


add_compile_definitions(ELAINEREFLECTION_EXPORTS)

add_library(ElaineReflection ${DIR_SRCS})

target_include_directories(ElaineReflection PUBLIC "Public")

