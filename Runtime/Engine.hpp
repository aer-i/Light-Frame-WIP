#pragma once
#include "Window.hpp"
#include "Renderer.hpp"
#include "Editor.hpp"

class Engine
{
public:
    Engine();
    ~Engine() = default;
    Engine(Engine&&) = delete;
    Engine(Engine const&) = delete;
    auto operator=(Engine&&) -> Engine& = delete;
    auto operator=(Engine const&) -> Engine& = delete;

    auto execute() -> void;

private:
    struct M
    {
        Window   window;
        Renderer renderer;
        Editor   editor;
    } m;
};