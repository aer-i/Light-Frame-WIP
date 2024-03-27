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
#include <imgui.h>

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
    auto initImgui()           -> void;
    auto terminateImgui()      -> void;

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

        vk::Image colorAttachment;
        vk::Image depthAttachment;
        vk::Image imguiFontTexture;

        vk::Buffer indirectBuffer;
        vk::Buffer meshIndexBuffer;
        vk::Buffer meshPositionBuffer;
        vk::Buffer meshNormalBuffer;
        vk::Buffer meshCoordsBuffer;
        vk::Buffer cameraUniformBuffer;
        vk::Buffer imguiIndexBuffer;
        vk::Buffer imguiVertexBuffer;
        vk::Buffer imguiDrawBuffer;
        vk::Buffer imguiIndirectBuffer;

        vk::Pipeline mainPipeline;
        vk::Pipeline gridPipeline;
        vk::Pipeline imguiPipeline;
        vk::Pipeline postProcessingPipeline;

        std::vector<vk::DrawIndirectCommand> indirectCommands;
    } m;
};