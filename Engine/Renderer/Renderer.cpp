#include "Renderer.hpp"
#include "Pipeline.hpp"
#include "Window.hpp"
#include <spdlog/spdlog.h>
#include <backends/imgui_impl_sdl3.h>

Renderer::Renderer(Window& window)
    : m{
        .window = window,
        .instance = vk::Instance{ false },
        .surface = vk::Surface{ window, m.instance },
        .physicalDevice = vk::PhysicalDevice{ m.instance },
        .device = vk::Device{ m.instance, m.surface, m.physicalDevice }
    }
{
    this->initImgui();
    this->allocateResources();
    this->createPipelines();

    spdlog::info("Created renderer");
}

Renderer::~Renderer()
{
    this->terminateImgui();
    spdlog::info("Destroyed renderer");
}

auto Renderer::renderFrame() -> void
{
    m.device.checkSwapchainState(m.window);
    m.device.waitForFences();
    m.device.acquireImage();

    auto& commands{ m.device.getCommandBuffer() };
    {
        commands.begin();

        commands.barrier(m.mainFramebuffer, vk::ImageLayout::eColorAttachment);
        commands.beginRendering(m.mainFramebuffer);

        auto cameraProjView{ m.currentCamera->getProjectionView() };

        commands.bindPipeline(m.mainPipeline);
        commands.pushConstant(&cameraProjView, sizeof cameraProjView);
        commands.draw(3);

        commands.endRendering();
        commands.barrier(m.mainFramebuffer, vk::ImageLayout::eShaderRead);

        commands.beginPresent();

        this->renderGui(commands);

        commands.endPresent();
        commands.end();
    }

    m.device.submitAndPresent();
}

auto Renderer::waitIdle() -> void
{
    m.device.waitIdle();
}

auto Renderer::resizeViewport(glm::uvec2 size) -> void
{
    m.device.waitIdle();

    m.mainFramebuffer.~Image();
    m.mainFramebuffer = vk::Image{
        &m.device,
        size,
        vk::ImageUsage::eColorAttachment | vk::ImageUsage::eSampled,
        vk::Format::eRGBA8_unorm
    };

    m.imguiPipeline.writeImage(m.mainFramebuffer, m.imguiViewportIndex, vk::DescriptorType::eCombinedImageSampler);
}

auto Renderer::setCamera(Camera* pCamera) -> void
{
    m.currentCamera = pCamera;
}

auto Renderer::getViewportTexture() -> u32
{
    return m.imguiViewportIndex;
}

auto Renderer::getWindow() -> Window&
{
    return m.window;
}

auto Renderer::initImgui() -> void
{
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(const_cast<SDL_Window*>(m.window.getHandle()));

    auto& io{ ImGui::GetIO() }; (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    auto fontData{ static_cast<u8*>(nullptr) };
    auto texWidth{ i32{} }, texHeight{ i32{} };

    io.Fonts->AddFontFromFileTTF("Assets/Fonts/Inter-SemiBold.ttf", 16.f);
    io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

    auto uploadSize{ size_t(texHeight * texWidth * 4) };

    m.imguiFontTexture = vk::Image(&m.device, {texWidth, texHeight}, vk::ImageUsage::eSampled, vk::Format::eRGBA8_unorm);
    m.imguiFontTexture.write(fontData, uploadSize);

    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    spdlog::info("Created ImGui context");
}

auto Renderer::terminateImgui() -> void
{
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    spdlog::info("Destroyed ImGui context");
}

auto Renderer::allocateResources() -> void
{
    {
        m.imguiVertexBuffer = vk::Buffer{ m.device, u32(512 * 1024 * sizeof(ImDrawVert)), vk::BufferUsage::eStorageBuffer, vk::MemoryType::eHost };
        m.imguiIndexBuffer  = vk::Buffer{ m.device, u32(512 * 1024 * sizeof(ImDrawIdx)),  vk::BufferUsage::eIndexBuffer,   vk::MemoryType::eHost };        
    }
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

    m.imguiPipeline = vk::Pipeline{ m.device, vk::Pipeline::Config{
        .point = vk::Pipeline::BindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .path = "shaders/imgui.vert.spv" },
            { .stage = vk::ShaderStage::eFragment, .path = "shaders/imgui.frag.spv" }
        },
        .descriptors = {
            { 0, vk::ShaderStage::eVertex, vk::DescriptorType::eStorageBuffer, &m.imguiVertexBuffer },
            { 1, vk::ShaderStage::eFragment, vk::DescriptorType::eCombinedImageSampler }
        },
        .topology = vk::Pipeline::Topology::eTriangleList,
        .cullMode = vk::Pipeline::CullMode::eNone,
        .colorBlending = true
    }};

    m.imguiFontTextureIndex = this->addImageToImgui(m.imguiFontTexture);
    m.imguiViewportIndex = this->addImageToImgui(m.mainFramebuffer);

    ImGui::GetIO().Fonts->SetTexID(&m.imguiFontTextureIndex);
}

auto Renderer::renderGui(vk::CommandBuffer& commands) -> void
{
    ImGui::Render();

    auto imDrawData{ ImGui::GetDrawData() };
    auto vertexOffset{ i32{} };
    auto indexOffset{ u32{} };

    if (imDrawData && imDrawData->CmdListsCount) [[likely]]
    {
        auto const vertexBufferSize{ imDrawData->TotalVtxCount * sizeof(ImDrawVert) };
        auto const indexBufferSize { imDrawData->TotalIdxCount * sizeof(ImDrawIdx)  };

        struct PushConstant
        {
            glm::vec2 scale;
            u32 textureIndex;
        } pushConstant;

        pushConstant.textureIndex = ~0u;
        pushConstant.scale = glm::vec2{
            2.f / imDrawData->DisplaySize.x,
            2.f / imDrawData->DisplaySize.y * -1
        };

        commands.bindPipeline(m.imguiPipeline);
        commands.bindIndexBuffer16(m.imguiIndexBuffer);

        for (auto cmdList : imDrawData->CmdLists)
        {
            m.imguiVertexBuffer.write(cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert), vertexOffset * sizeof(ImDrawVert));
            m.imguiIndexBuffer.write(cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx), indexOffset * sizeof(ImDrawIdx));

            for (auto& cmd : cmdList->CmdBuffer)
            {
                auto const offset{ glm::ivec2{ cmd.ClipRect.x < 0 ? 0 : cmd.ClipRect.x, cmd.ClipRect.y < 0 ? 0 : cmd.ClipRect.y }};
                auto const size{ glm::uvec2{cmd.ClipRect.z - cmd.ClipRect.x, cmd.ClipRect.w - cmd.ClipRect.y} };

                if (pushConstant.textureIndex != *reinterpret_cast<u32*>(cmd.TextureId))
                {
                    pushConstant.textureIndex = *reinterpret_cast<u32*>(cmd.TextureId);
                    commands.pushConstant(&pushConstant, sizeof(PushConstant));
                }

                commands.setScissor(offset, size);
                commands.drawIndexed(cmd.ElemCount, indexOffset, vertexOffset);

                indexOffset += cmd.ElemCount;
            }
            vertexOffset += cmdList->VtxBuffer.Size;
        }

        m.imguiVertexBuffer.flush(vertexBufferSize);
        m.imguiIndexBuffer.flush(indexBufferSize);
    }

    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
}

auto Renderer::addImageToImgui(vk::Image& image) -> u32
{
    auto static currentIndex{ u32{} };

    m.imguiPipeline.writeImage(image, currentIndex, vk::DescriptorType::eCombinedImageSampler);

    return currentIndex++;
}
