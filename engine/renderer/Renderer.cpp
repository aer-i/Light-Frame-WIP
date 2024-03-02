#include "Renderer.hpp"
#include <spdlog/spdlog.h>

Renderer::Renderer(Window& window)
    : m_window{ window }
    , m_instance{ true }
    , m_surface{ m_window, m_instance }
    , m_physicalDevice{ m_instance }
    , m_device{ m_instance, m_surface, m_physicalDevice }
{
    spdlog::info("Created renderer");
}

Renderer::~Renderer()
{
    spdlog::info("Destroyed renderer");
}
