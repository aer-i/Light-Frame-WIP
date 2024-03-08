#include "Engine.hpp"

Engine::Engine()
    : m{
        .renderer = Renderer{ m.window }
    }
{}

auto Engine::execute() -> void
{
    while (m.window.available())
    {
        m.window.update();

        m.renderer.renderFrame();
    }

    m.renderer.waitIdle();
}