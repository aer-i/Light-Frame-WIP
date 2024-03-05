#include "Renderer.hpp"
#include "Pipeline.hpp"
#include <spdlog/spdlog.h>

Renderer::Renderer(Window& window)
    : m_window{ window }
    , m_instance{ true }
    , m_surface{ m_window, m_instance }
    , m_physicalDevice{ m_instance }
    , m_device{ m_instance, m_surface, m_physicalDevice }
{
    spdlog::info("Created renderer");

    m_pipeline = vk::Pipeline{ m_device, vk::Pipeline::Config{
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
    m_device.checkSwapchainState(m_window);
    m_device.acquireImage();

    auto& commands{ m_device.getCommandBuffer() };

    commands.begin();
    commands.beginPresent();

    commands.bindPipeline(m_pipeline);
    commands.draw(3);

    commands.endPresent();
    commands.end();

    m_device.submitCommands(ArrayProxy<vk::CommandBuffer::Handle>{ commands });
    m_device.present();
}

auto Renderer::waitIdle() -> void
{
    m_device.waitIdle();
}
