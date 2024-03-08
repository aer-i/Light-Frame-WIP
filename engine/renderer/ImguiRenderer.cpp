#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ImguiRenderer.hpp"
#include "Window.hpp"
#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include <backends/imgui_impl_sdl3.h>
#include <spdlog/spdlog.h>
#include <stb_image_write.h>

ImguiRenderer::ImguiRenderer(Window& window,vk::Device& device)
    : m{
        .device = device
    }
{
    ImGui::CreateContext();

    auto& io{ ImGui::GetIO() }; (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL3_InitForVulkan(const_cast<SDL_Window*>(window.getHandle()));

    constexpr auto vertexBufferSize{ u32(512 * 1024 * sizeof(ImDrawVert)) };
    constexpr auto indexBufferSize { u32(512 * 1024 * sizeof(ImDrawIdx))  };

    m.vertexBuffer = vk::Buffer{ m.device, vertexBufferSize, vk::BufferUsage::eStorageBuffer, vk::MemoryType::eHost };
    m.indexBuffer  = vk::Buffer{ m.device, indexBufferSize,  vk::BufferUsage::eIndexBuffer,   vk::MemoryType::eHost };

    auto fontData{ (u8*)(nullptr) };
    auto texWidth{ i32{} }, texHeight{ i32{} };

    io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
    auto uploadSize{ size_t(texHeight * texWidth * 4) };

    m.fontTexture.allocate(&m.device, {texWidth, texHeight}, vk::ImageUsage::eSampled, vk::Format::eRGBA8_unorm);
    m.fontTexture.write(fontData, uploadSize);

    m.pipeline = vk::Pipeline{ m.device, vk::Pipeline::Config{
        .point = vk::Pipeline::BindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .path = "shaders/imgui.vert.spv" },
            { .stage = vk::ShaderStage::eFragment, .path = "shaders/imgui.frag.spv" }
        },
        .descriptors = {
            { 0, vk::ShaderStage::eVertex, vk::DescriptorType::eStorageBuffer, &m.vertexBuffer },
            { 1, vk::ShaderStage::eFragment, vk::DescriptorType::eCombinedImageSampler }
        },
        .topology = vk::Pipeline::Topology::eTriangleList,
        .cullMode = vk::Pipeline::CullMode::eNone,
        .colorBlending = true
    }};

    m.pipeline.writeImage(m.fontTexture, 0, vk::DescriptorType::eCombinedImageSampler);

    newFrame();

    spdlog::info("Created ImGui context");
}

ImguiRenderer::~ImguiRenderer()
{
    spdlog::info("Destroyed ImGui context");
}

auto ImguiRenderer::update() -> void
{
    ImGui::Render();
    auto imDrawData{ ImGui::GetDrawData() };

    auto vertexBufferSize{ imDrawData->TotalVtxCount * sizeof(ImDrawVert) };
    auto indexBufferSize { imDrawData->TotalIdxCount * sizeof(ImDrawIdx)  };

    if ((vertexBufferSize != 0) && (indexBufferSize != 0))
    {
        auto vtxOffset{ u32{} }, idxOffset{ u32{} };

        for (auto* cmdList : imDrawData->CmdLists)
        {
            m.vertexBuffer.write(cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert), vtxOffset);
            m.indexBuffer.write(cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx), idxOffset);
            vtxOffset += cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
            idxOffset += cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        }

        m.vertexBuffer.flush(vertexBufferSize);
        m.indexBuffer.flush(indexBufferSize);
    }
}

auto ImguiRenderer::renderGui(vk::CommandBuffer& commands) -> void
{
    auto imDrawData{ ImGui::GetDrawData() };
    auto vertexOffset{ i32{} };
    auto indexOffset{ u32{} };

    if ((!imDrawData) || (imDrawData->CmdListsCount == 0))
    {
        newFrame();
        return;
    }
    auto const scale{ glm::vec2{
        2.f / imDrawData->DisplaySize.x,
        2.f / imDrawData->DisplaySize.y * -1
    }};

    commands.bindPipeline(m.pipeline);
    commands.bindIndexBuffer16(m.indexBuffer);
    commands.pushConstant(&scale, sizeof(scale));

    for (auto cmdList : imDrawData->CmdLists)
    {
        for (auto& cmd : cmdList->CmdBuffer)
        {
            auto const offset{ glm::ivec2{ cmd.ClipRect.x < 0 ? 0 : cmd.ClipRect.x, cmd.ClipRect.y < 0 ? 0 : cmd.ClipRect.y }};
            auto const size{ glm::uvec2{cmd.ClipRect.z - cmd.ClipRect.x, cmd.ClipRect.w - cmd.ClipRect.y} };

            commands.setScissor(offset, size);
            commands.drawIndexed(cmd.ElemCount, indexOffset, vertexOffset);

            indexOffset += cmd.ElemCount;
        }
        vertexOffset += cmdList->VtxBuffer.Size;
    }

    newFrame();
}

auto ImguiRenderer::newFrame() -> void
{
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
}
