
set(lib 
 ElaineCore
 ElaineEngine
 glfw3
)

#include group
file(GLOB inc_com Include/*.h)
source_group("Header Files" FILES ${inc_com})
file(GLOB_RECURSE inc_imgui Include/imgui/*.h)
source_group(ImGui FILES ${inc_imgui})

#source group
file(GLOB src_com Source/*.cpp)
source_group("Source Files" FILES ${inc_com})
file(GLOB_RECURSE src_imgui Source/imgui/*.cpp)
source_group(ImGui FILES ${inc_imgui})



file(GLOB_RECURSE DIR_SRCS *.cpp)
file(GLOB_RECURSE DIR_INC *.h)
set(allFiles ${DIR_SRCS} ${DIR_INC})



add_executable(ElaineEditor ${DIR_SRCS})
target_sources(ElaineEditor PRIVATE ${DIR_INC})
target_include_directories(ElaineEditor PRIVATE ${THIRD_PARTY_DIR})
target_include_directories(ElaineEditor PRIVATE ${ENGINE_INCLUDE_DIR})
target_include_directories(ElaineEditor PUBLIC "Include")
target_include_directories(ElaineEditor PUBLIC "Include/imGui")
target_include_directories(ElaineEditor PUBLIC "Include/imGui/backends")

target_link_directories(ElaineEditor PRIVATE ${LIB_PATH})
target_link_directories(ElaineEditor PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/libs/x64/$<IF:$<CONFIG:EDITORRELEASE>,Release,$<IF:$<CONFIG:EDITORDEBUG>,Debug,$<IF:$<CONFIG:RUNTIMEDEBUG>,Debug,Release>>>)
if(_Use_Volk_)

else()
    target_link_directories(ElaineEditor PRIVATE  ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanSDK/lib/Windows)
    target_link_libraries(ElaineEditor PRIVATE "vulkan-1")
endif()

target_link_libraries(ElaineEditor PRIVATE ${lib})
add_dependencies(ElaineEditor ElaineCore ElaineEngine)
