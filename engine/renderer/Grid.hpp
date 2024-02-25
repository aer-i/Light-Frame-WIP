#pragma once
#include "Renderer.hpp"

class GridRenderer
{
public:
    GridRenderer(vk::Buffer& projViewBuffer)
        : m_projViewBuffer{ projViewBuffer }
    {
        this->createPipeline();
    }

    auto draw(vk::Cmd& cmd) -> void
    {
        cmd.bindPipeline(m_pipeline);
        cmd.draw(4);
    }

private:
    auto createPipeline() -> void
    {
        m_pipeline = vk::createPipeline(vk::PipelineConfig{
            .bindPoint = vk::PipelineBindPoint::eGraphics,
            .stages = {
                { .stage = vk::ShaderStage::eVertex,   .path = "shaders/grid.vert.spv" },
                { .stage = vk::ShaderStage::eFragment, .path = "shaders/grid.frag.spv" }
            },
            .descriptors = {
                { .binding = 0, .shaderStage = vk::ShaderStage::eVertex, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBuffer = &m_projViewBuffer }
            },
            .topology = vk::PipelineTopology::eTriangleFan,
            .cullMode = vk::PipelineCullMode::eBack,
            .colorBlending = true
        });
    }

private:
    vk::Buffer&  m_projViewBuffer;
    vk::Pipeline m_pipeline;
};