#include "Renderer.hpp"
#include "Pipeline.hpp"
#include "Window.hpp"
#include <spdlog/spdlog.h>

Renderer::Renderer(Window& window)
    : m{
        .window = window,
        .instance = vk::Instance{ true },
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
    switch (m.device.checkSwapchainState(m.window))
    {
    [[likely]]   case vk::Device::SwapchainResult::eSuccess:
        m.device.submitAndPresent();
        break;
    [[unlikely]] case vk::Device::SwapchainResult::eRecreated:
        this->onResize();
        m.device.submitAndPresent();
        break;
    [[unlikely]] case vk::Device::SwapchainResult::eTerminated:
        m.device.waitIdle();
        return;
    }
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
        {
            commands.barrier(m.mainFramebuffer, vk::ImageLayout::eColorAttachment);
            commands.beginRendering(m.mainFramebuffer);

            commands.bindPipeline(m.mainPipeline);
            commands.draw(3);

            commands.endRendering();
            commands.barrier(m.mainFramebuffer, vk::ImageLayout::eShaderRead);

            commands.beginPresent(i);

            commands.bindPipeline(m.postProcessingPipeline);
            commands.draw(4);

            commands.endPresent(i);
        }
        commands.end();

        ++i;
    }
}

auto Renderer::onResize() -> void
{
    this->waitIdle();

    m.mainFramebuffer.~Image();
    m.mainFramebuffer = vk::Image{
        &m.device,
        m.device.getExtent(),
        vk::ImageUsage::eColorAttachment | vk::ImageUsage::eSampled,
        vk::Format::eRGBA8_unorm
    };

    m.postProcessingPipeline.writeImage(m.mainFramebuffer, 0, vk::DescriptorType::eCombinedImageSampler);

    this->recordCommands();
}

auto Renderer::allocateResources() -> void
{
    m.mainFramebuffer = vk::Image{ 
        &m.device,
        m.device.getExtent(),
        vk::ImageUsage::eColorAttachment | vk::ImageUsage::eSampled,
        vk::Format::eRGBA8_unorm
    };
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

    m.postProcessingPipeline = vk::Pipeline{ m.device, vk::Pipeline::Config{
        .point = vk::Pipeline::BindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .path = "shaders/finalImage.vert.spv" },
            { .stage = vk::ShaderStage::eFragment, .path = "shaders/finalImage.frag.spv" },
        },
        .descriptors = {
            { 0, vk::ShaderStage::eFragment, vk::DescriptorType::eCombinedImageSampler }
        },
        .topology = vk::Pipeline::Topology::eTriangleFan
    }};

    m.postProcessingPipeline.writeImage(m.mainFramebuffer, 0, vk::DescriptorType::eCombinedImageSampler);
}