find_package(Vulkan REQUIRED)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL_DOCS ON CACHE BOOL "" FORCE)
set(SDL_INSTALL_TESTS OFF CACHE BOOL "" FORCE)

set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/SDL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/volk)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/assimp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/meshoptimizer)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/freetype)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator)

target_include_directories(LightFrame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/
    ${CMAKE_CURRENT_SOURCE_DIR}/aixlog/
    ${CMAKE_CURRENT_SOURCE_DIR}/stb/
    ${Vulkan_INCLUDE_DIR}/
)

target_sources(LightFrame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp
)

target_link_libraries(LightFrame PRIVATE
    GPUOpen::VulkanMemoryAllocator
    SDL3::SDL3-static
    meshoptimizer
    freetype
    glm::glm
    assimp
    volk
)