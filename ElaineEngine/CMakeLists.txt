
set(lib 
 ElaineCore 
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



file(GLOB_RECURSE DIR_SRCS *.cpp)
file(GLOB_RECURSE DIR_INC *.h)

add_compile_definitions(EAGLEENGINE_EXPORTS)
add_library(ElaineEngine ${DIR_SRCS})
add_dependencies(ElaineEngine ElaineCore)
target_sources(ElaineEngine PRIVATE ${DIR_INC})
target_include_directories(ElaineEngine PRIVATE ${THIRD_PARTY_DIR})
target_include_directories(ElaineEngine PRIVATE ${ENGINE_INCLUDE_DIR})
target_include_directories(
    ElaineEngine
    PUBLIC 
    "Public"
    "Public/Common"
    "Public/GamePlay")

target_link_directories(ElaineEngine PRIVATE ${LIB_PATH})
target_link_libraries(ElaineEngine ${lib})