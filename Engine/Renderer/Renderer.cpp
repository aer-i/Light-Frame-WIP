#include "Renderer.hpp"
#include "Pipeline.hpp"
#include "Window.hpp"
#include <spdlog/spdlog.h>

Renderer::Renderer(Window& window)
    : m{
        .window = window,
        .instance = vk::Instance{ false },
        .surface = vk::Surface{ window, m.instance },
        .physicalDevice = vk::PhysicalDevice{ m.instance },
        .device = vk::Device{ m.instance, m.surface, m.physicalDevice }
    }
{
    this->allocateResources();
    this->createPipelines();

    this->recordCommands();

    spdlog::info("Created renderer");
}

Renderer::~Renderer()
{
    spdlog::info("Destroyed renderer");
}

auto Renderer::renderFrame() -> void
{
    if (m.device.checkSwapchainState(m.window)) [[unlikely]]
    {
        m.device.waitIdle();
        this->recordCommands();
    }

    m.device.waitForFences();
    m.device.acquireImage();
    m.device.submitAndPresent();
}

auto Renderer::waitIdle() -> void
{
    m.device.waitIdle();
}

auto Renderer::setCamera(Camera* pCamera) -> void
{
    m.currentCamera = pCamera;
}

auto Renderer::getWindow() -> Window&
{
    return m.window;
}

auto Renderer::recordCommands() -> void
{
    for (auto i{ u32{} }; auto& commands : m.device.getCommandBuffers())
    {
        commands.begin();

        commands.beginPresent(i);

        commands.bindPipeline(m.mainPipeline);
        commands.draw(3);

        commands.endPresent(i);
        commands.end();

        ++i;
    }
}

auto Renderer::allocateResources() -> void
{
    {
        m.mainFramebuffer = vk::Image{ 
            &m.device,
            { 1920, 1080 },
            vk::ImageUsage::eColorAttachment | vk::ImageUsage::eSampled,
            vk::Format::eRGBA8_unorm
        };
    }
}

auto Renderer::createPipelines() -> void
{
    m.mainPipeline = vk::Pipeline{ m.device, vk::Pipeline::Config{
        .point = vk::Pipeline::BindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .path = "shaders/triangle.vert.spv" },
            { .stage = vk::ShaderStage::eFragment, .path = "shaders/triangle.frag.spv" },
        },
        .topology = vk::Pipeline::Topology::eTriangleList,
    }};
}