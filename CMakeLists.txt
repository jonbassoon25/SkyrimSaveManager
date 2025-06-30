# Based off template from https://github.com/SkyrimScripting/SKSE_Template_HelloWorld

cmake_minimum_required(VERSION 3.21)
project(SkyrimSaveManager VERSION 0.1.0 LANGUAGES CXX)

set(OUTPUT_FOLDER "${CMAKE_SOURCE_DIR}/Distro/SkyrimSaveManager")

# Setup your SKSE plugin as an SKSE plugin!
find_package(CommonLibSSE CONFIG REQUIRED)
add_commonlibsse_plugin(${PROJECT_NAME} SOURCES SaveManager.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${PROJECT_NAME} PRIVATE PCH.h)

# Copy the .dll from build to the specified output folder
if(DEFINED OUTPUT_FOLDER)
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_SOURCE_DIR}/SaveManager.ini"
            "${DLL_FOLDER}/SaveManager.ini"
        VERBATIM
    )
endif()
