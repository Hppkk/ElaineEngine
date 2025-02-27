cmake_minimum_required(VERSION 3.26 FATAL_ERROR)
project(ElanieEngine)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)

set(_Use_Volk_  OFF)

set(CMAKE_CONFIGURATION_TYPES EditorDebug EditorRelease RuntimeDebug RuntimeRelease)
set(CMAKE_SHARED_LINKER_FLAGS_EDITORDEBUG ${CMAKE_SHARED_LINKER_FLAGS_DEBUG})
set(CMAKE_SHARED_LINKER_FLAGS_EDITORRELEASE ${CMAKE_SHARED_LINKER_FLAGS_RELEASE})
set(CMAKE_SHARED_LINKER_FLAGS_RUNTIMEDEBUG ${CMAKE_SHARED_LINKER_FLAGS_DEBUG})
set(CMAKE_SHARED_LINKER_FLAGS_RUNTIMERELEASE ${CMAKE_SHARED_LINKER_FLAGS_RELEASE})
set(CMAKE_EXE_LINKER_FLAGS_EDITORDEBUG ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
set(CMAKE_EXE_LINKER_FLAGS_EDITORRELEASE ${CMAKE_EXE_LINKER_FLAGS_RELEASE})
set(CMAKE_EXE_LINKER_FLAGS_RUNTIMEDEBUG ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
set(CMAKE_EXE_LINKER_FLAGS_RUNTIMERELEASE ${CMAKE_EXE_LINKER_FLAGS_RELEASE})


set(CMAKE_CXX_FLAGS_EDITORDEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_HAS_EDITOR_")
set(CMAKE_CXX_FLAGS_EDITORRELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_HAS_EDITOR_")
set(CMAKE_CXX_FLAGS_RUNTIMEDEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RUNTIMERELEASE "${CMAKE_CXX_FLAGS_RELEASE}")





# set compiler
if(WIN32)
    set(CMAKE_CXX_COMPILER "MSVC")
    add_compile_options(/wd4251)

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ElaineEditor)
    set(PlatformTarget Win64)
    add_compile_options(/W3 /WX /MP)
    add_compile_definitions(_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)
    add_definitions(-D_UNICODE)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT  "EditAndContinue")
elseif(UNIX)
    set(CMAKE_CXX_COMPILER "g++")
    set(PlatformTarget Linux)
elseif(ANDROID)
    set(CMAKE_CXX_COMPILER "clang")
    set(PlatformTarget Android)
elseif(IOS)
    set(CMAKE_CXX_COMPILER "clang")
    set(PlatformTarget IOS)
endif()

message("Target OS : ${PlatformTarget}")
message("Compiler : ${CMAKE_CXX_COMPILER}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS ON)
include(CMakeDependentOption)

set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Binaries/${PlatformTarget})
set(CMAKE_PDB_OUTPUT_DIRECTORY_EDITORDEBUG ${CMAKE_CURRENT_SOURCE_DIR}/Binaries/${PlatformTarget}/EditorPDB)
set(CMAKE_PDB_OUTPUT_DIRECTORY_RUNTIMEDEBUG ${CMAKE_CURRENT_SOURCE_DIR}/Binaries/${PlatformTarget}/RuntimePDB)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Binaries/${PlatformTarget})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Binaries/${PlatformTarget})
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Binaries/${PlatformTarget})
set(ThirdPartyDir ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty)

set(THIRD_PARTY_DIR 
 ${ThirdPartyDir}/cJSON 
 ${ThirdPartyDir}/glfw/include 
 ${ThirdPartyDir}/spdlog/include 
 ${ThirdPartyDir}/VulkanMemoryAllocator/include 
 ${ThirdPartyDir}/VulkanSDK/include/vulkan 
 ${ThirdPartyDir}/VulkanSDK/include 

)

set(ENGINE_INCLUDE_DIR 
 ${CMAKE_CURRENT_SOURCE_DIR}/ElaineCore/Include 
 ${CMAKE_CURRENT_SOURCE_DIR}/ElaineCore/Include/Common 
 ${CMAKE_CURRENT_SOURCE_DIR}/ElaineCore/Include/component 
 ${CMAKE_CURRENT_SOURCE_DIR}/ElaineCore/Include/JobSystem
 ${CMAKE_CURRENT_SOURCE_DIR}/ElaineCore/Include/math
 ${CMAKE_CURRENT_SOURCE_DIR}/ElaineCore/Include/render 
 ${CMAKE_CURRENT_SOURCE_DIR}/ElaineCore/Include/resource
 ${CMAKE_CURRENT_SOURCE_DIR}/ElaineCore/Include/platform 
 ${CMAKE_CURRENT_SOURCE_DIR}/ElaineEngine/Include 
 ${CMAKE_CURRENT_SOURCE_DIR}/ElaineEngine/Include/common 
 ${CMAKE_CURRENT_SOURCE_DIR}/ElaineEngine/Include/component 

)

add_subdirectory(ElaineCore)
add_subdirectory(ElaineEditor)
add_subdirectory(ElaineEngine)
add_subdirectory(Samples)
