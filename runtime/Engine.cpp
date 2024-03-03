#include "Engine.hpp"

Engine::Engine()
    : m_window{ }
    , m_renderer{ m_window }
{}

auto Engine::execute() -> void
{
    while (m_window.available())
    {
        m_window.update();

        m_renderer.renderFrame();
    }
}