find_package(Vulkan REQUIRED)

set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/volk)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator)

target_include_directories(LightFrame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/
    ${CMAKE_CURRENT_SOURCE_DIR}/aixlog/
    ${Vulkan_INCLUDE_DIR}/
)

target_link_libraries(LightFrame PRIVATE
    GPUOpen::VulkanMemoryAllocator
    glm::glm
    glfw
    volk
)