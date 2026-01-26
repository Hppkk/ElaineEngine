
set(lib 
 ElaineCore
 ElaineEngine
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

file(GLOB_RECURSE DIR_SRCS *.cpp)
file(GLOB_RECURSE DIR_INC *.h)
set(allFiles ${DIR_SRCS} ${DIR_INC})

if(_Use_Volk_)
    add_compile_definitions(USE_VOLK)
    add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
endif()

if(_Use_Volk_)
    add_executable(ElaineEditor ${DIR_SRCS}
    ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanSDK/include/Volk/volk.c)
else()
    add_executable(ElaineEditor ${DIR_SRCS})
endif()

target_sources(ElaineEditor PRIVATE ${DIR_INC})
target_include_directories(ElaineEditor PRIVATE ${THIRD_PARTY_DIR})
target_include_directories(ElaineEditor PRIVATE ${ENGINE_INCLUDE_DIR})
target_include_directories(
    ElaineEditor 
    PRIVATE 
    "Public"
    "Public/imgui"
    "Public/imgui/backends"
    )

target_link_directories(ElaineEditor PRIVATE ${LIB_PATH})
target_link_directories(ElaineEditor PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/libs/x64/$<IF:$<CONFIG:EDITORRELEASE>,Release,$<IF:$<CONFIG:EDITORDEBUG>,Debug,$<IF:$<CONFIG:RUNTIMEDEBUG>,Debug,Release>>>)
if(_Use_Volk_)

else()
    target_link_directories(ElaineEditor PRIVATE  ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanSDK/lib/Windows)
    target_link_libraries(ElaineEditor PRIVATE "vulkan-1")
endif()

target_link_libraries(ElaineEditor PRIVATE ${lib})
add_dependencies(ElaineEditor ElaineCore ElaineEngine)
