
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
        Public/
    PRIVATE
        ../ThirdParty/libclang/include)

target_link_libraries(
    ElaineReflection
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/libclang/bin/libclang.lib)

add_custom_command(
    TARGET ElaineReflection PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/libclang/bin/libclang.dll"
        $<TARGET_FILE_DIR:ElaineReflection>
)

