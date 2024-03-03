#pragma once
#include "Instance.hpp"
#include "Surface.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"

class Window;

class Renderer
{
public:
    Renderer(Window& window);
    ~Renderer();

public:
    auto renderFrame() -> void;

private:
    Window&            m_window;
    vk::Instance       m_instance;
    vk::Surface        m_surface;
    vk::PhysicalDevice m_physicalDevice;
    vk::Device         m_device;
};