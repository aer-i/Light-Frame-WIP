cmake_minimum_required(VERSION 3.15)

project(LightFrame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LF_ENGINE_SOURCES
Engine/Renderer/Vulkan/Instance.cpp
Engine/Renderer/Vulkan/Surface.cpp
Engine/Renderer/Vulkan/PhysicalDevice.cpp
Engine/Renderer/Vulkan/Device.cpp
Engine/Renderer/Vulkan/CommandBuffer.cpp
Engine/Renderer/Vulkan/Pipeline.cpp
Engine/Renderer/Vulkan/Image.cpp
Engine/Renderer/Vulkan/Buffer.cpp
Engine/Renderer/Renderer.cpp
Engine/Renderer/Window.cpp
Engine/Renderer/Camera.cpp
Engine/Editor/Editor.cpp
Engine/Editor/Viewport.cpp
Engine/Scene/MeshLoader.cpp
)

set(LF_ENGINE_DIRS
Engine/Renderer/Vulkan/
Engine/Renderer/
Engine/Editor/
Engine/Core/
Engine/Scene/
Engine/./
)

file(GLOB_RECURSE LF_RUNTIME_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Runtime/*.cpp)

add_executable(LightFrame)
target_sources(LightFrame PRIVATE ${LF_ENGINE_SOURCES} ${LF_RUNTIME_SOURCES})
target_include_directories(LightFrame PRIVATE ${LF_ENGINE_DIRS})

include(${CMAKE_CURRENT_SOURCE_DIR}/vendor.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/assets.cmake)

if (MSVC)
    target_compile_definitions(LightFrame PUBLIC _CRT_SECURE_NO_WARNINGS)
    #set_target_properties(LightFrame PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()