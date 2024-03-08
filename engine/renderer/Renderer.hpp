#pragma once
#include "Instance.hpp"
#include "Surface.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "ImguiRenderer.hpp"

class Window;

class Renderer
{
public:
    Renderer(Window& window);
    ~Renderer();
    Renderer(Renderer const&) = delete;
    Renderer(Renderer&&) = delete;
    auto operator=(Renderer const&) -> Renderer& = delete;
    auto operator=(Renderer&&) -> Renderer& = delete;

public:
    auto renderFrame() -> void;
    auto renderGui()   -> void;
    auto waitIdle()    -> void;

private:
    struct M
    {
        Window&            window;
        vk::Instance       instance;
        vk::Surface        surface;
        vk::PhysicalDevice physicalDevice;
        vk::Device         device;
        vk::Pipeline       pipeline;
        ImguiRenderer      imguiRenderer;
    } m;
};