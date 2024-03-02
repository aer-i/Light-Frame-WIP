#include "Surface.hpp"
#include "Window.hpp"
#include "Instance.hpp"
#include <volk.h>
#include <SDL3/SDL_vulkan.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

vk::Surface::Surface(Window& window, Instance& instance)
    : m_instance{ &instance }
    , m_surface{ nullptr }
{
    if (!SDL_Vulkan_CreateSurface(const_cast<SDL_Window*>(window.getHandle()), *m_instance, nullptr, &m_surface))
    {
        throw std::runtime_error("Failed to create window surface");
    }

    spdlog::info("Created window surface");
}

vk::Surface::~Surface()
{
    if (m_surface)
    {
        vkDestroySurfaceKHR(*m_instance, m_surface, nullptr);
        m_surface = nullptr;
    }

    spdlog::info("Destroyed window surface");
}

vk::Surface::Surface(Surface&& other)
    : m_instance{ other.m_instance }
    , m_surface{ other.m_surface }
{
    other.m_instance = nullptr;
    other.m_surface  = nullptr;
}

auto vk::Surface::operator=(Surface&& other) -> Surface&
{
    m_instance = other.m_instance;
    m_surface  = other.m_surface;

    other.m_instance = nullptr;
    other.m_surface  = nullptr;

    return *this;
}