#include "Renderer.hpp"
#include "Pipeline.hpp"
#include <spdlog/spdlog.h>

Renderer::Renderer(Window& window)
    : m{
        .window = window,
        .instance = vk::Instance{ true },
        .surface = vk::Surface{ window, m.instance },
        .physicalDevice = vk::PhysicalDevice{ m.instance },
        .device = vk::Device{ m.instance, m.surface, m.physicalDevice },
        .imguiRenderer = ImguiRenderer{ window, m.device }
    }
{
    spdlog::info("Created renderer");

    m.pipeline = vk::Pipeline{ m.device, vk::Pipeline::Config{
        .point = vk::Pipeline::BindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .path = "shaders/triangle.vert.spv" },
            { .stage = vk::ShaderStage::eFragment, .path = "shaders/triangle.frag.spv" },
        },
        .topology = vk::Pipeline::Topology::eTriangleList,
    }};
}

Renderer::~Renderer()
{
    spdlog::info("Destroyed renderer");
}

auto Renderer::renderFrame() -> void
{
    m.device.checkSwapchainState(m.window);
    m.device.acquireImage();

    auto& commands{ m.device.getCommandBuffer() };

    commands.begin();
    commands.beginPresent();

    commands.bindPipeline(m.pipeline);
    commands.draw(3);

    ImGui::ShowDemoWindow();

    m.imguiRenderer.update();
    m.imguiRenderer.renderGui(commands);

    commands.endPresent();
    commands.end();

    m.device.submitCommands(ArrayProxy<vk::CommandBuffer::Handle>{ commands });
    m.device.present();
}

auto Renderer::renderGui() -> void
{
    
}

auto Renderer::waitIdle() -> void
{
    m.device.waitIdle();
}
