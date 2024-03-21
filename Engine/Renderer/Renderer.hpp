#pragma once
#include "Instance.hpp"
#include "Surface.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"
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

public:
    auto renderFrame()                   -> void;
    auto waitIdle()                      -> void;
    auto resizeViewport(glm::uvec2 size) -> void;
    auto setCamera(Camera* pCamera)      -> void;
    auto getViewportTexture()            -> u32;
    auto getWindow()                     -> Window&;

private:
    auto initImgui() -> void;
    auto terminateImgui() -> void;
    auto allocateResources() -> void;
    auto createPipelines() -> void;
    auto renderGui(vk::CommandBuffer& commands) -> void;
    auto addImageToImgui(vk::Image& image) -> u32;

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

        vk::Pipeline imguiPipeline;
        vk::Buffer   imguiVertexBuffer;
        vk::Buffer   imguiIndexBuffer;
        vk::Image    imguiFontTexture;
        u32          imguiFontTextureIndex;
        u32          imguiViewportIndex;
    } m;
};