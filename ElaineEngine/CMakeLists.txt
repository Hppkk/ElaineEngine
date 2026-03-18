
set(lib 
 ElaineCore 
 ElaineReflection
)

#include group
file(GLOB inc_ Public/*.h)
source_group("Public" FILES ${inc_})
file(GLOB_RECURSE inc_com Public/Common/*.h)
source_group(Public/Common FILES ${inc_com})
file(GLOB_RECURSE inc_component Public/GamePlay/*.h)
source_group(Public/GamePlay FILES ${inc_component})

#source group
file(GLOB src_ Private/*.cpp)
source_group("Private" FILES ${src_})
file(GLOB_RECURSE src_com Private/Common/*.cpp)
source_group(Private/Common FILES ${src_com})
file(GLOB_RECURSE src_component Private/GamePlay/*.cpp)
source_group(Private/GamePlay FILES ${src_component})



file(GLOB_RECURSE DIR_SRCS CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE DIR_INC CONFIGURE_DEPENDS *.h)

add_compile_definitions(EAGLEENGINE_EXPORTS)
add_library(ElaineEngine ${DIR_SRCS})
add_dependencies(ElaineEngine ElaineCore ElaineReflection)
target_sources(ElaineEngine PRIVATE ${DIR_INC})
target_include_directories(ElaineEngine PRIVATE ${THIRD_PARTY_DIR})
target_include_directories(ElaineEngine PRIVATE ${ENGINE_INCLUDE_DIR})
target_include_directories(
    ElaineEngine
    PUBLIC 
    "Public"
    "Public/Common"
    "Public/GamePlay"
    "Generated")

target_link_directories(ElaineEngine PRIVATE ${LIB_PATH})
target_link_libraries(ElaineEngine ${lib})

# Pre-build step: run ElaineBuildTool to generate reflection code
add_custom_command(
    TARGET ElaineEngine PRE_BUILD
    COMMAND $<TARGET_FILE:ElaineBuildTool>
        --input "${CMAKE_CURRENT_SOURCE_DIR}/Public"
        --output "${CMAKE_CURRENT_SOURCE_DIR}/Generated"
        --editor-output "${CMAKE_SOURCE_DIR}/ElaineEditor/Generated"
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
    COMMENT "[ElaineBuildTool] Generating reflection code..."
    DEPENDS ElaineBuildTool
)