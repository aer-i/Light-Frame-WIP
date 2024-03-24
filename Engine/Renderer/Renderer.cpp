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
    this->loadModel("Assets/Models/kitten.obj");

    this->allocateResources();
    this->createPipelines();
    this->recordCommandsEmpty();

    spdlog::info("Created renderer");
}

Renderer::~Renderer()
{
    spdlog::info("Destroyed renderer");
}

auto Renderer::updateBuffers() -> void
{
    struct
    {
        glm::mat4 projection, view, projView;
    } cameraData{
        .projection = m.currentCamera->getProjection(),
        .view = m.currentCamera->getView(),
        .projView = m.currentCamera->getProjectionView()
    };

    m.cameraUnfiromBuffer.write(&cameraData, sizeof(cameraData));
    m.cameraUnfiromBuffer.flush(m.cameraUnfiromBuffer.getSize());
}

auto Renderer::recordCommandsEmpty() -> void
{
    for (auto i{ u32{} }; auto& commands : m.device.getCommandBuffers())
    {
        commands.begin();
        {
            commands.beginPresent(i);
            commands.endPresent(i);
        }
        commands.end();

        ++i;
    }
}

auto Renderer::recordCommands() -> void
{
    for (auto i{ u32{} }; auto& commands : m.device.getCommandBuffers())
    {
        commands.begin();
        {
            commands.barrier(m.colorAttachment, vk::ImageLayout::eColorAttachment);
            commands.barrier(m.depthAttachment, vk::ImageLayout::eDepthAttachment);
            commands.beginRendering(m.colorAttachment, &m.depthAttachment);

            commands.bindPipeline(m.mainPipeline);
            commands.drawIndirect(m.indirectBuffer, static_cast<u32>(m.indirectCommands.size()));

            commands.endRendering();
            commands.barrier(m.colorAttachment, vk::ImageLayout::eShaderRead);

            commands.beginPresent(i);

            commands.bindPipeline(m.postProcessingPipeline);
            commands.draw(3);

            commands.endPresent(i);
        }
        commands.end();

        ++i;
    }
}

auto Renderer::onResize() -> void
{
    this->waitIdle();

    m.colorAttachment.~Image();
    m.colorAttachment = vk::Image{
        &m.device,
        m.device.getExtent(),
        vk::ImageUsage::eColorAttachment | vk::ImageUsage::eSampled,
        vk::Format::eRGBA8_unorm
    };

    m.depthAttachment.~Image();
    m.depthAttachment = vk::Image{
        &m.device,
        m.device.getExtent(),
        vk::ImageUsage::eDepthAttachment,
        vk::Format::eD32_sfloat
    };

    m.postProcessingPipeline.writeImage(m.colorAttachment, 0, vk::DescriptorType::eCombinedImageSampler);

    this->recordCommands();
}

auto Renderer::allocateResources() -> void
{
    m.colorAttachment = vk::Image{ 
        &m.device,
        m.device.getExtent(),
        vk::ImageUsage::eColorAttachment | vk::ImageUsage::eSampled,
        vk::Format::eRGBA8_unorm
    };

    m.depthAttachment = vk::Image{
        &m.device,
        m.device.getExtent(),
        vk::ImageUsage::eDepthAttachment,
        vk::Format::eD32_sfloat
    };

    m.indirectBuffer = vk::Buffer{
        m.device,
        static_cast<u32>(sizeof(vk::IndirectDrawCommand) * 1024),
        vk::BufferUsage::eIndirectBuffer,
        vk::MemoryType::eDevice
    };

    m.indirectBuffer.write(m.indirectCommands.data(), m.indirectCommands.size() * sizeof(vk::IndirectDrawCommand));

    m.cameraUnfiromBuffer = vk::Buffer{
        m.device,
        sizeof(glm::mat4) * 3,
        vk::BufferUsage::eUniformBuffer,
        vk::MemoryType::eHost
    };

    m.meshIndexBuffer = vk::Buffer{
        m.device,
        static_cast<u32>(m.meshLoader.indices.size() * sizeof(u32)),
        vk::BufferUsage::eStorageBuffer,
        vk::MemoryType::eDevice
    };

    m.meshIndexBuffer.write(m.meshLoader.indices.data(), m.meshLoader.indices.size() * sizeof(m.meshLoader.indices[0]));

    m.meshPositionBuffer = vk::Buffer{
        m.device,
        static_cast<u32>(m.meshLoader.positions.size() * sizeof(u32)),
        vk::BufferUsage::eStorageBuffer,
        vk::MemoryType::eDevice
    };

    m.meshPositionBuffer.write(m.meshLoader.positions.data(), m.meshLoader.positions.size() * sizeof(m.meshLoader.positions[0]));

    m.meshCoordsBuffer = vk::Buffer{
        m.device,
        static_cast<u32>(m.meshLoader.uvs.size() * sizeof(u32)),
        vk::BufferUsage::eStorageBuffer,
        vk::MemoryType::eDevice
    };

    m.meshCoordsBuffer.write(m.meshLoader.uvs.data(), m.meshLoader.uvs.size() * sizeof(m.meshLoader.uvs[0]));

    m.meshNormalBuffer = vk::Buffer{
        m.device,
        static_cast<u32>(m.meshLoader.normals.size() * sizeof(u32)),
        vk::BufferUsage::eStorageBuffer,
        vk::MemoryType::eDevice
    };

    m.meshNormalBuffer.write(m.meshLoader.normals.data(), m.meshLoader.normals.size() * sizeof(m.meshLoader.normals[0]));
}

auto Renderer::createPipelines() -> void
{
    m.mainPipeline = vk::Pipeline{ m.device, vk::Pipeline::Config{
        .point = vk::Pipeline::BindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .path = "shaders/main.vert.spv" },
            { .stage = vk::ShaderStage::eFragment, .path = "shaders/main.frag.spv" }
        },
        .descriptors = {
            { 0, vk::ShaderStage::eVertex, vk::DescriptorType::eStorageBuffer, &m.meshIndexBuffer },
            { 1, vk::ShaderStage::eVertex, vk::DescriptorType::eStorageBuffer, &m.meshPositionBuffer },
            { 2, vk::ShaderStage::eVertex, vk::DescriptorType::eStorageBuffer, &m.meshCoordsBuffer },
            { 3, vk::ShaderStage::eVertex, vk::DescriptorType::eStorageBuffer, &m.meshNormalBuffer },
            { 4, vk::ShaderStage::eVertex, vk::DescriptorType::eUniformBuffer, &m.cameraUnfiromBuffer }
        },
        .topology = vk::Pipeline::Topology::eTriangleList,
        .cullMode = vk::Pipeline::CullMode::eBack,
        .depthWrite = true
    }};

    m.postProcessingPipeline = vk::Pipeline{ m.device, vk::Pipeline::Config{
        .point = vk::Pipeline::BindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .path = "shaders/finalImage.vert.spv" },
            { .stage = vk::ShaderStage::eFragment, .path = "shaders/finalImage.frag.spv" }
        },
        .descriptors = {
            { 0, vk::ShaderStage::eFragment, vk::DescriptorType::eCombinedImageSampler }
        },
        .topology = vk::Pipeline::Topology::eTriangleFan,
        .cullMode = vk::Pipeline::CullMode::eNone
    }};

    m.postProcessingPipeline.writeImage(m.colorAttachment, 0, vk::DescriptorType::eCombinedImageSampler);
}

auto Renderer::renderFrame() -> void
{
    this->updateBuffers();

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

auto Renderer::loadModel(std::string_view path) -> void
{
    auto meshes{ m.meshLoader.loadMesh(path, false) };

    for (auto const& mesh : meshes)
    {
        auto instance{ static_cast<u32>(m.indirectCommands.size()) };

        m.indirectCommands.emplace_back(vk::IndirectDrawCommand{
            .vertexCount = mesh.indexCount,
            .instanceCount = 1,
            .firstVertex = mesh.indexOffset,
            .firstInstance = instance
        });
    }
}
