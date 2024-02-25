#pragma once
#include "Font.hpp"

class TextRenderer
{
public:
    TextRenderer(Font& font)
        : m_font{ font }
    {
        m_vertices.reserve(512 * 1024);
        m_vertices.emplace_back();

        m_buffer.allocate(512 * 1024 * sizeof(Vertex), vk::BufferUsage::eStorageBuffer | vk::BufferUsage::eIndirectBuffer, vk::MemoryType::eHost);

        m_pipeline = vk::createPipeline(vk::PipelineConfig{
            .bindPoint = vk::PipelineBindPoint::eGraphics,
            .stages = {
                { .stage = vk::ShaderStage::eVertex,   .path = "shaders/font2d.vert.spv" },
                { .stage = vk::ShaderStage::eFragment, .path = "shaders/font2d.frag.spv" }
            },
            .descriptors = {
                { 0, vk::ShaderStage::eVertex, vk::DescriptorType::eStorageBuffer, &m_buffer, sizeof(vk::DrawIndirectCommands), m_buffer.size - sizeof(vk::DrawIndirectCommands) },
                { 1, vk::ShaderStage::eFragment, vk::DescriptorType::eCombinedImageSampler }
            },
            .topology = vk::PipelineTopology::eTriangleFan,
            .cullMode = vk::PipelineCullMode::eBack,
            .colorBlending = true
        });
        
        m_pipeline.writeImage(&font.texture, 0, vk::DescriptorType::eCombinedImageSampler);
        this->flush();
    }

    auto render2D(std::string_view text, glm::vec2 pos, f32 scale = 1.f) -> void
    {
        for (auto c = text.begin(); *c; ++c)
        {
            auto ch = m_font.characters[*c];

            float x2 =    pos.x + ch.left * scale;
            float y2 = -(-pos.y - ch.top  * scale);
            float w  = ch.width       * scale;
            float h  = ch.height      * scale;

            pos.x += ch.advanceX * scale;
            pos.y += ch.advanceY * scale;

            if (!w || !h)
            {
                continue;
            }

            m_vertices.push_back({ x2,     y2 - h, ch.offsetX,                                 ch.height / m_font.atlasSize.y });
            m_vertices.push_back({ x2 + w, y2 - h, ch.offsetX + ch.width / m_font.atlasSize.x, ch.height / m_font.atlasSize.y });
            m_vertices.push_back({ x2 + w, y2,     ch.offsetX + ch.width / m_font.atlasSize.x, 0.f                          });
            m_vertices.push_back({ x2,     y2,     ch.offsetX,                                 0.f                          });
        }
    }

    auto flush() -> void
    {
        auto drawCommand{ vk::DrawIndirectCommands{
            .vertexCount = 4,
            .instanceCount = static_cast<u32>((m_vertices.size() - 1) / 4),
            .firstVertex = 0,
            .firstInstance = 0
        }};

        m_vertices[0] = *reinterpret_cast<Vertex*>(&drawCommand);

        m_buffer.writeData(m_vertices.data(), m_vertices.size() * sizeof(Vertex));
        m_vertices.resize(1);
    }

    auto recordCommands(vk::Cmd& cmd) -> void
    {
        cmd.bindPipeline(m_pipeline);
        cmd.drawIndirect(m_buffer, 0, 1);
    }

private:
    struct Vertex{ f32 x, y, u, v; };

    Font& m_font;
    vk::Pipeline m_pipeline;
    vk::Buffer   m_buffer;
    std::vector<Vertex> m_vertices;
};