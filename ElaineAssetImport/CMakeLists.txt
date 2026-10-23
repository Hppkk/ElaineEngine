add_library(ElaineAssetImport STATIC
    Private/ElaineTextureImporter.cpp
    Private/ElaineMeshImporter.cpp
    Private/ElaineResourceImportService.cpp)
target_include_directories(ElaineAssetImport PUBLIC Public PRIVATE ${THIRD_PARTY_DIR} ${ThirdPartyDir}/assimp/include)
# Keep the importer dependency out of the runtime resource library.
add_library(ElaineAssimp SHARED IMPORTED GLOBAL)
set_target_properties(ElaineAssimp PROPERTIES
    IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/ThirdParty/libs/x64/Release/assimp-vc145-mt.lib"
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/ThirdParty/libs/x64/Release/assimp-vc145-mt.dll")
foreach(config DEBUG EDITORDEBUG RUNTIMEDEBUG)
    set_target_properties(ElaineAssimp PROPERTIES
        IMPORTED_IMPLIB_${config} "${CMAKE_SOURCE_DIR}/ThirdParty/libs/x64/Debug/assimp-vc145-mtd.lib"
        IMPORTED_LOCATION_${config} "${CMAKE_SOURCE_DIR}/ThirdParty/libs/x64/Debug/assimp-vc145-mtd.dll")
endforeach()
target_link_libraries(ElaineAssetImport PUBLIC ElaineCore PRIVATE ElaineAssimp)
# Every importing tool/test runs from the common binary output directory.
add_custom_command(TARGET ElaineAssetImport POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:ElaineAssimp>" "$<TARGET_FILE_DIR:ElaineAssetImport>"
    VERBATIM)
if(MSVC)
    target_compile_options(ElaineAssetImport PRIVATE /wd4819 /wd4828 /wd4996 /wd4275)
endif()
