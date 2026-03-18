
set(lib 
 ElaineCore
 ElaineEngine
 ElaineReflection
 d3d11
 d3dcompiler
 dxgi
 glfw3
)

#include group
file(GLOB inc_com Public/*.h)
source_group("Public" FILES ${inc_com})
file(GLOB_RECURSE inc_imgui Public/imgui/*.h)
source_group(ImGui FILES ${inc_imgui})

#source group
file(GLOB src_com Private/*.cpp)
source_group("Private" FILES ${src_com})
file(GLOB_RECURSE src_imgui Private/Imgui/*.cpp)
source_group(ImGui FILES ${src_imgui})

file(GLOB_RECURSE DIR_SRCS CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE DIR_INC CONFIGURE_DEPENDS *.h)
set(allFiles ${DIR_SRCS} ${DIR_INC})

add_executable(ElaineEditor ${DIR_SRCS})

target_sources(ElaineEditor PRIVATE ${DIR_INC})
target_include_directories(ElaineEditor PRIVATE ${THIRD_PARTY_DIR})
target_include_directories(ElaineEditor PRIVATE ${ENGINE_INCLUDE_DIR})
target_include_directories(
    ElaineEditor 
    PRIVATE 
    "Public"
    "Public/imgui"
    "Public/imgui/backends"
    "Public/imgui/ImGuizmo"
    "Generated"
    )

target_link_directories(ElaineEditor PRIVATE ${LIB_PATH})
target_link_directories(ElaineEditor PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/libs/x64/$<IF:$<CONFIG:EDITORRELEASE>,Release,$<IF:$<CONFIG:EDITORDEBUG>,Debug,$<IF:$<CONFIG:RUNTIMEDEBUG>,Debug,Release>>>)

target_link_libraries(ElaineEditor PRIVATE ${lib})
add_dependencies(ElaineEditor ElaineCore ElaineEngine)

# Pre-build step: run ElaineBuildTool to generate editor code
# This ensures .editor.generated.cpp files exist before ElaineEditor compiles
add_custom_command(
    TARGET ElaineEditor PRE_BUILD
    COMMAND $<TARGET_FILE:ElaineBuildTool>
        --input "${CMAKE_SOURCE_DIR}/ElaineEngine/Public"
        --output "${CMAKE_SOURCE_DIR}/ElaineEngine/Generated"
        --editor-output "${CMAKE_CURRENT_SOURCE_DIR}/Generated"
        --include "${CMAKE_SOURCE_DIR}/ElaineCore/Include"
        --include "${CMAKE_SOURCE_DIR}/ElaineCore/Include/Common"
        --include "${CMAKE_SOURCE_DIR}/ElaineCore/Include/math"
        --include "${CMAKE_SOURCE_DIR}/ElaineCore/Include/render"
        --include "${CMAKE_SOURCE_DIR}/ElaineCore/Include/platform"
        --include "${CMAKE_SOURCE_DIR}/ElaineCore/Include/Resource"
        --include "${CMAKE_SOURCE_DIR}/ElaineCore/Include/File"
        --include "${CMAKE_SOURCE_DIR}/ElaineCore/Include/JobSystem"
        --include "${CMAKE_SOURCE_DIR}/ElaineEngine/Public"
        --include "${CMAKE_SOURCE_DIR}/ElaineEngine/Public/Common"
        --include "${CMAKE_SOURCE_DIR}/ElaineEngine/Public/GamePlay"
        --include "${CMAKE_SOURCE_DIR}/ElaineReflection/Public"
        --include "${CMAKE_SOURCE_DIR}/ThirdParty"
        --include "${CMAKE_SOURCE_DIR}/ThirdParty/json/include"
    COMMENT "[ElaineBuildTool] Generating editor code..."
    DEPENDS ElaineBuildTool
)
