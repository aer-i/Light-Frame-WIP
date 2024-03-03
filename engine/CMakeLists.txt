set(ENGINE_SOURCES
"renderer/vulkan/Instance.cpp"
"renderer/vulkan/Surface.cpp"
"renderer/vulkan/PhysicalDevice.cpp"
"renderer/vulkan/Device.cpp"
"renderer/vulkan/Image.cpp"
"renderer/Renderer.cpp"
"renderer/Window.cpp"
#"renderer/ImguiRenderer.cpp"
#"renderer/Font.cpp"
#"scene/MeshLoader.cpp"
)

set(ENGINE_DIRS
"renderer/vulkan/"
"renderer/"
"core/"
"scene/"
)

target_sources(LightFrame PRIVATE ${ENGINE_SOURCES})
target_include_directories(LightFrame PRIVATE ${ENGINE_DIRS})