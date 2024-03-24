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

private:
    auto updateBuffers()     -> void;
    auto recordCommands()    -> void;
    auto onResize()          -> void;
    auto allocateResources() -> void;
    auto createPipelines()   -> void;

public:
    inline auto setCamera(Camera* pCamera) -> void
    {
        m.currentCamera = pCamera;
    }

    inline auto getWindow() -> Window&
    {
        return m.window;
    }

private:
    struct M
    {
        Window& window;
        Camera* currentCamera;

        vk::Instance       instance;
        vk::Surface        surface;
        vk::PhysicalDevice physicalDevice;
        vk::Device         device;

        vk::Image mainFramebuffer;

        vk::Buffer cameraUnfiromBuffer;

        vk::Pipeline mainPipeline;
        vk::Pipeline postProcessingPipeline;
    } m;
};