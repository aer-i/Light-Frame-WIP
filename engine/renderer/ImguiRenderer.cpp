#include "ImguiRenderer.hpp"
#include "Window.hpp"
#include <backends/imgui_impl_sdl3.h>
#include <memory>
#include <cstring>

namespace vk::imgui
{
    static auto g_pipeline    { vk::Pipeline{} };
    static auto g_fontTexture { vk::Image{}    };
    static auto g_vertexBuffer{ vk::Buffer{}   };
    static auto g_indexBuffer { vk::Buffer{}   };
    static auto g_offsetBuffer{ vk::Buffer{}   };
    static auto g_drawBuffer  { vk::Buffer{}   };

    auto inline static newFrame() -> void
    {
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    }

    auto init() -> void
    {
        ImGui::CreateContext();

        auto& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui_ImplSDL3_InitForVulkan(const_cast<SDL_Window*>(Window::GetHandle()));

        constexpr auto vertexBufferSize{ u32(512 * 1024 * sizeof(ImDrawVert)) };
        constexpr auto indexBufferSize { u32(512 * 1024 * sizeof(ImDrawIdx))  };

        u8* fontData;
        i32 texWidth, texHeight;

        io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
        auto uploadSize{ size_t(texHeight * texWidth * 4) };

        g_vertexBuffer.allocate(512 * 1024 * sizeof(ImDrawVert), vk::BufferUsage::eStorageBuffer, vk::MemoryType::eHost);
        g_indexBuffer.allocate(512 * 1024 * sizeof(ImDrawIdx), vk::BufferUsage::eStorageBuffer, vk::MemoryType::eHost);
        g_drawBuffer.allocate(512 * 1024 * sizeof(DrawIndirectCommands), vk::BufferUsage::eIndirectBuffer, vk::MemoryType::eHost);
        g_offsetBuffer.allocate(512 * 1024 * sizeof(u32), vk::BufferUsage::eStorageBuffer, vk::MemoryType::eHost);
        g_fontTexture.allocate({texWidth, texHeight}, vk::ImageUsage::eSampled, Format::eRGBA8_unorm);
        g_fontTexture.write(fontData, uploadSize);

        g_pipeline = vk::createPipeline(PipelineConfig{
            .bindPoint = PipelineBindPoint::eGraphics,
            .stages = {
                { .stage = vk::ShaderStage::eVertex,   .path = "shaders/imgui.vert.spv"},
                { .stage = vk::ShaderStage::eFragment, .path = "shaders/imgui.frag.spv"}
            },
            .descriptors = {
                { 0, ShaderStage::eVertex,   DescriptorType::eStorageBuffer, &g_vertexBuffer },
                { 1, ShaderStage::eVertex,   DescriptorType::eStorageBuffer, &g_indexBuffer },
                { 3, ShaderStage::eVertex,   DescriptorType::eStorageBuffer, &g_offsetBuffer },
                { 2, ShaderStage::eFragment, DescriptorType::eCombinedImageSampler }
            },
            .topology = vk::PipelineTopology::eTriangleList,
            .cullMode = vk::PipelineCullMode::eNone,
            .colorBlending = true
        });

        g_pipeline.writeImage(&g_fontTexture, 0, DescriptorType::eCombinedImageSampler);

        newFrame();
    }

    auto teardown() -> void
    {
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    auto update() -> void
    {
        // going insane rn
        return;
        ImGui::Render();
        auto imDrawData = ImGui::GetDrawData();

        {
            auto vertexBufferSize{ imDrawData->TotalVtxCount * sizeof(ImDrawVert) };
            auto indexBufferSize { imDrawData->TotalIdxCount * sizeof(ImDrawIdx)  };

            if ((vertexBufferSize != 0) && (indexBufferSize != 0))
            {
                auto vtxOffset{ u32{} };
                auto idxOffset{ u32{} };

                auto veritices{ std::vector<ImDrawVert>{} };
                auto indices  { std::vector<ImDrawIdx> {} };
                veritices.reserve(imDrawData->TotalVtxCount);
                indices.reserve(imDrawData->TotalIdxCount);

                for (auto* cmdList : imDrawData->CmdLists)
                {
                    veritices.insert(veritices.end(), cmdList->VtxBuffer.begin(), cmdList->VtxBuffer.end());
                    indices.insert(indices.end(), cmdList->IdxBuffer.begin(), cmdList->IdxBuffer.end());
                }

                g_vertexBuffer.writeData(veritices.data(), vertexBufferSize);
                g_indexBuffer.writeData(indices.data(), indexBufferSize);
            }
        }
        {
            u32 vertexOffset = 0;
            u32 indexOffset = 0;
            u32 drawCount = 0;
            struct Draw{ u32 io, vo; };
            std::vector<Draw> draws;

		    if (imDrawData && imDrawData->CmdListsCount > 0)
            {
                for (auto* cmdList : imDrawData->CmdLists)
                {
                    for (auto& cmd : cmdList->CmdBuffer)
                    {
                        ++drawCount;
                        draws.emplace_back(indexOffset, vertexOffset);

                        indexOffset += cmd.ElemCount;
                    }
                    vertexOffset += cmdList->VtxBuffer.Size;
                }

                g_offsetBuffer.writeData(draws.data(), draws.size() * sizeof(draws[0]));
		    }

            auto drawCommand{ DrawIndirectCommands{
                .vertexCount = (u32)imDrawData->TotalIdxCount,
                .instanceCount = drawCount,
                .firstVertex = 0,
                .firstInstance = 0
            }};

            g_drawBuffer.writeData(&drawCommand, sizeof(drawCommand));
        }

        newFrame();
    }

    auto render(Cmd& cmd) -> void
    {
        //cmd.bindPipeline(g_pipeline);
        //cmd.drawIndirect(g_drawBuffer, 0, 1);
        //cmd.drawIndirectCount(g_drawBuffer, 1024 * 256);
    }
}