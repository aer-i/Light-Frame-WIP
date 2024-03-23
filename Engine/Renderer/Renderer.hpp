#pragma once
#include "Instance.hpp"
#include "Surface.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"

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
    auto renderFrame()                   -> void;
    auto waitIdle()                      -> void;
    auto setCamera(Camera* pCamera)      -> void;
    auto getWindow()                     -> Window&;

private:
    auto recordCommands()    -> void;
    auto allocateResources() -> void;
    auto createPipelines()   -> void;

private:
    struct M
    {
        Window& window;
        Camera* currentCamera;

        vk::Instance       instance;
        vk::Surface        surface;
        vk::PhysicalDevice physicalDevice;
        vk::Device         device;

        vk::Pipeline mainPipeline;
        vk::Image    mainFramebuffer;
    } m;
};