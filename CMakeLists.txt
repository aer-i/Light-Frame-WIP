cmake_minimum_required(VERSION 3.15)

project(LightFrame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(LightFrame)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/engine)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/runtime)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor)

#compile shader to bin directory-------------------------------------

find_program(GLSL_VALIDATOR glslangValidator HINTS
    ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
    /usr/bin
    /usr/local/bin
    ${VULKAN_SDK_PATH}/Bin
    ${VULKAN_SDK_PATH}/Bin32
    $ENV{VULKAN_SDK}/Bin/
    $ENV{VULKAN_SDK}/Bin32/
)

if (NOT GLSL_VALIDATOR)
    message(FATAL_ERROR "GLSL Validator not found")
endif ()

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "shaders/*.frag"
    "shaders/*.vert"
    "shaders/*.comp"
    "shaders/*.geom"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "shaders/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "shaders/"
        COMMAND ${GLSL_VALIDATOR} --target-env vulkan1.3 -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(LightFrame Shaders)