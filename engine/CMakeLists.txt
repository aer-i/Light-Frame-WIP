set(ENGINE_SOURCES
"renderer/Renderer.cpp"
"renderer/Window.cpp"
)

set(ENGINE_DIRS
"renderer/"
"core/"
)

target_sources(LightFrame PRIVATE ${ENGINE_SOURCES})
target_include_directories(LightFrame PRIVATE ${ENGINE_DIRS})