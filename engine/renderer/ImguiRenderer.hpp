#pragma once
#include "Buffer.hpp"
#include "Image.hpp"
#include "Pipeline.hpp"
#include <imgui.h>

namespace vk
{
    class CommandBuffer;
}

class Window;

class ImguiRenderer
{
public:
    ImguiRenderer(Window& window, vk::Device& device);
    ~ImguiRenderer();
    ImguiRenderer(ImguiRenderer const&) = delete;
    ImguiRenderer(ImguiRenderer&&) = delete;
    auto operator=(ImguiRenderer const&) -> ImguiRenderer& = delete;
    auto operator=(ImguiRenderer&&) -> ImguiRenderer& = delete;

public:
    auto update() -> void;
    auto renderGui(vk::CommandBuffer& commands) -> void;

private:
    auto newFrame() -> void;

private:
    struct M
    {
        vk::Device&  device;
        vk::Pipeline pipeline;
        vk::Buffer   vertexBuffer;
        vk::Buffer   indexBuffer;
        vk::Image    fontTexture;
    } m;
};