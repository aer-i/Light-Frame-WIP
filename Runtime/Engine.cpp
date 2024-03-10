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
        m.window.update();
        m.editor.render();
        m.renderer.renderFrame();
    }

    m.renderer.waitIdle();
}