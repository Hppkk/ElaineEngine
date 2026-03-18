
file(GLOB_RECURSE publicFiles Public/*.h)
source_group("Public" FILES ${publicFiles})
file(GLOB_RECURSE privateFiles Private/*.cpp)
source_group("Private" FILES ${privateFiles})


file(GLOB_RECURSE DIR_SRCS *.cpp)
file(GLOB_RECURSE DIR_INC *.h)


add_compile_definitions(ELAINEREFLECTION_EXPORTS)

add_library(ElaineReflection ${DIR_SRCS})

target_include_directories(
    ElaineReflection 
    PUBLIC 
        Public/)

# Need nlohmann/json for serialization (header-only)
target_include_directories(
    ElaineReflection
    PRIVATE
        ${CMAKE_SOURCE_DIR}/ThirdParty/json/include
        ${CMAKE_SOURCE_DIR}/ThirdParty)

target_sources(ElaineReflection PRIVATE ${DIR_INC})

