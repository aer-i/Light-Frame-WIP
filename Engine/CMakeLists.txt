set(ENGINE_SOURCES
"Renderer/Vulkan/Instance.cpp"
"Renderer/Vulkan/Surface.cpp"
"Renderer/Vulkan/PhysicalDevice.cpp"
"Renderer/Vulkan/Device.cpp"
"Renderer/Vulkan/CommandBuffer.cpp"
"Renderer/Vulkan/Pipeline.cpp"
"Renderer/Vulkan/Image.cpp"
"Renderer/Vulkan/Buffer.cpp"
"Renderer/Renderer.cpp"
"Renderer/Window.cpp"
"Renderer/Camera.cpp"
"Editor/Editor.cpp"
"Editor/Viewport.cpp"
"Scene/MeshLoader.cpp"
)

set(ENGINE_DIRS
"Renderer/Vulkan/"
"Renderer/"
"Editor/"
"Core/"
"Scene/"
"./"
)

target_sources(LightFrame PRIVATE ${ENGINE_SOURCES})
target_include_directories(LightFrame PRIVATE ${ENGINE_DIRS})