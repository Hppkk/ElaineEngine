

set(lib 
 cjson
 glfw3
 spdlog 
)

set(core_lib_path 
 ${CMAKE_SOURCE_DIR}/ThirdParty/libs/x64/$<IF:$<CONFIG:EDITORRELEASE>,Release,$<IF:$<CONFIG:EDITORDEBUG>,Debug,$<IF:$<CONFIG:RUNTIMEDEBUG>,Debug,Release>>>
 ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanSDK/lib/Windows
)

if(_Use_Volk_)
    add_compile_definitions(USE_VOLK)
    add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
endif()

#incldue group
file(GLOB inc_ Include/*.h)
source_group("Header Files" FILES ${inc_})
file(GLOB_RECURSE inc_commom Include/Common/*.h)
source_group(Common FILES ${inc_commom})
file(GLOB_RECURSE inc_job Include/JobSystem/*.h)
source_group(JobSystem FILES ${inc_job})
file(GLOB_RECURSE inc_math Include/math/*.h)
source_group(math FILES ${inc_math})
file(GLOB inc_render Include/render/*.h)
source_group(render FILES ${inc_render})
file(GLOB_RECURSE inc_render_com Include/render/common/*.h)
source_group(render/common FILES ${inc_render_com})
file(GLOB_RECURSE inc_render_vulkan Include/render/vulkan/*.h)
source_group(render/vulkan FILES ${inc_render_vulkan})
file(GLOB_RECURSE inc_render_dx12 Include/render/dx12/*.h)
source_group(render/dx12 FILES ${inc_render_dx12})
file(GLOB_RECURSE inc_file Include/File/*.h)
source_group(File FILES ${inc_file})
file(GLOB_RECURSE inc_resource Include/Resource/*.h)
source_group(Resource FILES ${inc_resource})
file(GLOB inc_platform_com Include/platform/*.h)
source_group(platform FILES ${inc_platform_com})
file(GLOB_RECURSE inc_platform_win Include/platform/Windows/*.h)
source_group(platform/Windows FILES ${inc_platform_win})
file(GLOB_RECURSE inc_task_graph Include/TaskGraph/*.h)
source_group(TaskGraph FILES ${inc_task_graph})
file(GLOB_RECURSE inc_render_graph Include/RenderGraph/*.h)
source_group(RenderGraph FILES ${inc_render_graph})
file(GLOB_RECURSE inc_post_process Include/PostProcess/*.h)
source_group(PostProcess FILES ${inc_post_process})
file(GLOB_RECURSE inc_ren_proxy Include/RenderProxy/*.h)
source_group(RenderProxy FILES ${inc_ren_proxy})

#source group
file(GLOB src_ Source/*.cpp)
source_group("Source Files" FILES ${src_})
file(GLOB_RECURSE src_commom Source/Common/*.cpp)
source_group(Common FILES ${src_commom})
file(GLOB_RECURSE src_job Source/JobSystem/*.cpp)
source_group(JobSystem FILES ${src_job})
file(GLOB_RECURSE src_math Source/math/*.cpp)
source_group(math FILES ${src_math})
file(GLOB src_render Source/render/*.cpp)
source_group(render FILES ${src_render})
file(GLOB_RECURSE src_render_com Source/render/common/*.cpp)
source_group(render/common FILES ${src_render_com})
file(GLOB_RECURSE src_render_vulkan Source/render/vulkan/*.cpp)
source_group(render/vulkan FILES ${src_render_vulkan})
file(GLOB_RECURSE src_render_dx12 Source/render/dx12/*.cpp)
source_group(render/dx12 FILES ${src_render_dx12})
file(GLOB_RECURSE src_file Source/File/*.cpp)
source_group(File FILES ${src_file})
file(GLOB_RECURSE src_resource Source/Resource/*.cpp)
source_group(Resource FILES ${src_resource})
file(GLOB_RECURSE src_platform_win Source/platform/Windows/*.cpp)
source_group(platform/Windows FILES ${src_platform_win})
file(GLOB_RECURSE src_task_graph Source/TaskGraph/*.cpp)
source_group(TaskGraph FILES ${src_task_graph})
file(GLOB_RECURSE src_render_graph Source/RenderGraph/*.cpp)
source_group(RenderGraph FILES ${src_render_graph})
file(GLOB_RECURSE src_post_process Source/PostProcess/*.cpp)
source_group(PostProcess FILES ${src_post_process})
file(GLOB_RECURSE src_ren_proxy Source/RenderProxy/*.cpp)
source_group(RenderProxy FILES ${src_ren_proxy})

file(GLOB_RECURSE DIR_SRCS *.cpp)
file(GLOB_RECURSE DIR_INC *.h)

set(PreHeader_CPP "ElainePrecompiledHeader.cpp")
set(PreHeader_H "ElainePrecompiledHeader.h")

if (MSVC)
    set_directory_properties(PROPERTIES COMPILE_OPTIONS "/Yu${PreHeader_H}")
    set_source_files_properties(${PreHeader_CPP}
        PROPERTIES
        COMPILE_FLAGS "/Yc${PreHeader_H}"
        )
    foreach( src_file ${DIR_SRCS} )
        if("${CMAKE_CURRENT_SOURCE_DIR}/Source/Common/${PreHeader_CPP}" STREQUAL ${src_file})
            set_source_files_properties(
                ${src_file}
                PROPERTIES
                COMPILE_FLAGS "/Yc${PreHeader_H}"   
            )
        else()
            set_source_files_properties(
                ${src_file}
                PROPERTIES
                COMPILE_FLAGS "/Yu${PreHeader_H}"   
            )
        endif()
    endforeach( src_file ${DIR_SRCS} )
    #list(APPEND DIR_SRCS "${PreHeader_CPP}")
endif (MSVC)

add_compile_definitions(ELAINECORE_DLLEXPORT)

if(_Use_Volk_)
    set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanSDK/include/Volk/volk.c
        PROPERTIES
        COMPILE_FLAGS "/Y-"   
    )
    add_library(ElaineCore ${DIR_SRCS}
    ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanSDK/include/Volk/volk.c)
else()
    add_library(ElaineCore ${DIR_SRCS})
endif()

target_sources(ElaineCore PRIVATE ${DIR_INC})
target_include_directories(ElaineCore PUBLIC ${ThirdPartyDir}/json/include)
target_include_directories(ElaineCore PUBLIC ${ThirdPartyDir}/assimp/include)
target_include_directories(ElaineCore PRIVATE ${THIRD_PARTY_DIR})
target_include_directories(ElaineCore PUBLIC "Include")
target_include_directories(ElaineCore PUBLIC "Include/Common")
target_include_directories(ElaineCore PUBLIC "Include/JobSystem")
target_include_directories(ElaineCore PUBLIC "Include/math")
target_include_directories(ElaineCore PUBLIC "Include/render")
target_include_directories(ElaineCore PUBLIC "Include/Resource")
target_include_directories(ElaineCore PUBLIC "Include/platform")
target_include_directories(ElaineCore PUBLIC "Include/File")
target_include_directories(ElaineCore PUBLIC "Include/PostProcess")
target_include_directories(ElaineCore PUBLIC "Include/TaskGraph")
target_include_directories(ElaineCore PUBLIC "Include/RenderGraph")

if(_Use_Volk_)

else()
    target_link_directories(ElaineCore PRIVATE  ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanSDK/lib/Windows)
    target_link_libraries(ElaineCore PRIVATE "vulkan-1")
endif()


target_link_directories(ElaineCore PRIVATE ${core_lib_path})
target_link_libraries(ElaineCore PRIVATE ${lib})
add_dependencies(ElaineCore)