#include "Engine.hpp"

Engine::Engine()
    : m{
        .renderer = Renderer{ m.window },
        .editor = Editor{ m.renderer }
    }
{}

auto Engine::execute() -> void
{
    while (m.window.available())
    {
        m.renderer.beginFrame();
        m.editor.render();
        m.window.update();
        m.renderer.endFrame();
    }

    m.renderer.waitIdle();
}