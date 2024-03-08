
set(lib 
 ElaineCore 
)

#include group
file(GLOB inc_ Include/*.h)
source_group("Header Files" FILES ${inc_})
file(GLOB_RECURSE inc_com Include/common/*.h)
source_group(common FILES ${inc_com})
file(GLOB_RECURSE inc_component Include/component/*.h)
source_group(component FILES ${inc_component})

#source group
file(GLOB src_ Source/*.cpp)
source_group("Source Files" FILES ${src_})
file(GLOB_RECURSE src_com Source/common/*.cpp)
source_group(common FILES ${src_com})
file(GLOB_RECURSE src_component Include/component/*.cpp)
source_group(component FILES ${src_component})



file(GLOB_RECURSE DIR_SRCS *.cpp)
file(GLOB_RECURSE DIR_INC *.h)

add_compile_definitions(EAGLEENGINE_EXPORTS)
add_library(ElaineEngine ${DIR_SRCS})
add_dependencies(ElaineEngine ElaineCore)
target_sources(ElaineEngine PRIVATE ${DIR_INC})
target_include_directories(ElaineEngine PRIVATE ${THIRD_PARTY_DIR})
target_include_directories(ElaineEngine PRIVATE ${ENGINE_INCLUDE_DIR})
target_link_directories(ElaineEngine PRIVATE ${LIB_PATH})
target_link_libraries(ElaineEngine ${lib})