#pragma once
#include "Instance.hpp"
#include "Surface.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"
#include "MeshLoader.hpp"
#include <memory>

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

private:
    auto updateBuffers()       -> void;
    auto recordCommandsEmpty() -> void;
    auto recordCommands()      -> void;
    auto onResize()            -> void;
    auto allocateResources()   -> void;
    auto createPipelines()     -> void;

public:
    auto renderFrame()                    -> void;
    auto waitIdle()                       -> void;
    auto loadModel(std::string_view path) -> void;

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
        Window&    window;
        Camera*    currentCamera;
        MeshLoader meshLoader;

        vk::Instance       instance;
        vk::Surface        surface;
        vk::PhysicalDevice physicalDevice;
        vk::Device         device;

        vk::Image mainFramebuffer;

        vk::Buffer indirectBuffer;
        vk::Buffer meshIndexBuffer;
        vk::Buffer meshPositionBuffer;
        vk::Buffer meshNormalBuffer;
        vk::Buffer meshCoordsBuffer;
        vk::Buffer cameraUnfiromBuffer;

        vk::Pipeline mainPipeline;
        vk::Pipeline postProcessingPipeline;

        std::vector<vk::IndirectDrawCommand> indirectCommands;
    } m;
};