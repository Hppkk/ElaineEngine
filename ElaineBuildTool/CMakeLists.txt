
file(GLOB_RECURSE publicFiles Public/*.h Public/*.cpp)
source_group("Public" FILES ${publicFiles})
file(GLOB_RECURSE privateFiles Private/*.cpp)
source_group("Private" FILES ${privateFiles})

file(GLOB_RECURSE DIR_SRCS *.cpp)
file(GLOB_RECURSE DIR_INC *.h)

add_executable(ElaineBuildTool ${DIR_SRCS})

target_sources(ElaineBuildTool PRIVATE ${DIR_INC})

target_include_directories(
    ElaineBuildTool
    PRIVATE
        Public/
        ../ThirdParty/libclang/include)

target_link_libraries(
    ElaineBuildTool
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/libclang/bin/libclang.lib)

# Copy libclang.dll next to the executable
add_custom_command(
    TARGET ElaineBuildTool POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/libclang/bin/libclang.dll"
        $<TARGET_FILE_DIR:ElaineBuildTool>
)

# Set the C++ standard explicitly
set_target_properties(ElaineBuildTool PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# No precompiled headers for the build tool
if (MSVC)
    target_compile_options(ElaineBuildTool PRIVATE /Y-)
endif()
