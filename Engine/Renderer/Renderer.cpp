#include "Renderer.hpp"
#include "Pipeline.hpp"
#include "Window.hpp"
#include <spdlog/spdlog.h>
#include <backends/imgui_impl_sdl3.h>

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

    this->initImgui();
    this->allocateResources();
    this->createPipelines();
    this->recordCommandsEmpty();

    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    spdlog::info("Created renderer");
}

Renderer::~Renderer()
{
    this->terminateImgui();
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

    m.cameraUniformBuffer.write(&cameraData, sizeof(cameraData));
    m.cameraUniformBuffer.flush(m.cameraUniformBuffer.getSize());

    ImGui::ShowDemoWindow();

    {
        ImGui::Render();
        auto imDrawData{ ImGui::GetDrawData() };

        auto const vertexBufferSize{ imDrawData->TotalVtxCount * sizeof(ImDrawVert) };
        auto const indexBufferSize { imDrawData->TotalIdxCount * sizeof(ImDrawIdx)  };

        if ((vertexBufferSize != 0) && (indexBufferSize != 0))
        {
            auto vtxOffset{ u32{} };
            auto idxOffset{ u32{} };

            for (auto* cmdList : imDrawData->CmdLists)
            {
                m.imguiVertexBuffer.write(cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert), vtxOffset);
                m.imguiIndexBuffer.write(cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx), idxOffset);

                vtxOffset += cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
                idxOffset += cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
            }

            m.imguiVertexBuffer.flush(vertexBufferSize);
            m.imguiIndexBuffer.flush(indexBufferSize);
        }

        auto vertexOffset{ i32{} };
        auto indexOffset{ u32{} };
        auto drawCount{ u32{} };

        if (imDrawData && imDrawData->CmdListsCount > 0)
        {
            for (auto* cmdList : imDrawData->CmdLists)
            {
                for (auto& cmd : cmdList->CmdBuffer)
                {
                    auto const drawCommand{ vk::DrawIndexedIndirectCommand{
                        .indexCount = cmd.ElemCount,
                        .instanceCount = 1,
                        .firstIndex = indexOffset,
                        .vertexOffset = vertexOffset,
                        .firstInstance = drawCount
                    }};

                    m.imguiDrawBuffer.write(&cmd.ClipRect, sizeof(cmd.ClipRect), sizeof(cmd.ClipRect) * drawCount);
                    m.imguiIndirectBuffer.write(&drawCommand, sizeof(drawCommand), sizeof(u32) + drawCount * sizeof(drawCommand));

                    ++drawCount;
                    indexOffset += cmd.ElemCount;
                }
                vertexOffset += cmdList->VtxBuffer.Size;
            }

            m.imguiIndirectBuffer.write(&drawCount, sizeof(drawCount));
            m.imguiIndirectBuffer.flush(sizeof(u32) + drawCount * sizeof(vk::DrawIndexedIndirectCommand));
        }

        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    }
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

            commands.bindPipeline(m.gridPipeline);
            commands.draw(4);

            commands.bindPipeline(m.mainPipeline);
            commands.drawIndirect(m.indirectBuffer, static_cast<u32>(m.indirectCommands.size()));

            commands.endRendering();
            commands.barrier(m.colorAttachment, vk::ImageLayout::eShaderRead);

            commands.beginPresent(i);

            commands.bindPipeline(m.postProcessingPipeline);
            commands.draw(3);

            commands.bindPipeline(m.imguiPipeline);
            commands.bindIndexBuffer16(m.imguiIndexBuffer);
            commands.drawIndexedIndirectCount(m.imguiIndirectBuffer, 1024);

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

    {
        auto fontData{ static_cast<u8*>(nullptr) };
        auto texWidth{ i32{} }, texHeight{ i32{} };

        ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
        auto const uploadSize{ static_cast<size_t>(texHeight * texWidth * 4) };

        m.imguiFontTexture = vk::Image{
            &m.device,
            { texWidth, texHeight },
            vk::ImageUsage::eSampled,
            vk::Format::eRGBA8_unorm
        };

        m.imguiFontTexture.write(fontData, uploadSize);
    }

    m.indirectBuffer = vk::Buffer{
        m.device,
        static_cast<u32>(sizeof(vk::DrawIndirectCommand) * 1024),
        vk::BufferUsage::eIndirectBuffer,
        vk::MemoryType::eDevice
    };

    m.indirectBuffer.write(m.indirectCommands.data(), m.indirectCommands.size() * sizeof(vk::DrawIndirectCommand));

    m.cameraUniformBuffer = vk::Buffer{
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

    {
        constexpr auto vertexBufferSize  { u32{ 512 * 1024 * sizeof(ImDrawVert)}                     };
        constexpr auto indexBufferSize   { u32{ 512 * 1024 * sizeof(ImDrawIdx)}                      };
        constexpr auto indirectBufferSize{ u32{ 512 * 1024 * sizeof(vk::DrawIndexedIndirectCommand)} };

        auto const initDrawCount{ u32{} };

        m.imguiVertexBuffer   = vk::Buffer{ m.device, vertexBufferSize,   vk::BufferUsage::eStorageBuffer,  vk::MemoryType::eHost };
        m.imguiIndexBuffer    = vk::Buffer{ m.device, indexBufferSize,    vk::BufferUsage::eIndexBuffer,    vk::MemoryType::eHost };
        m.imguiDrawBuffer     = vk::Buffer{ m.device, indexBufferSize,    vk::BufferUsage::eStorageBuffer,    vk::MemoryType::eHost };
        m.imguiIndirectBuffer = vk::Buffer{ m.device, indirectBufferSize, vk::BufferUsage::eIndirectBuffer, vk::MemoryType::eHost };

        m.imguiIndirectBuffer.write(&initDrawCount, sizeof(initDrawCount));
        m.imguiIndirectBuffer.flush(m.imguiIndirectBuffer.getSize());
    }

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
            { 4, vk::ShaderStage::eVertex, vk::DescriptorType::eUniformBuffer, &m.cameraUniformBuffer }
        },
        .topology = vk::Pipeline::Topology::eTriangleList,
        .cullMode = vk::Pipeline::CullMode::eBack,
        .depthWrite = true,
        .depthTest = true,
    }};

    m.gridPipeline = vk::Pipeline{ m.device, vk::Pipeline::Config{
        .point = vk::Pipeline::BindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .path = "shaders/grid.vert.spv" },
            { .stage = vk::ShaderStage::eFragment, .path = "shaders/grid.frag.spv" }
        },
        .descriptors = {
            { 0, vk::ShaderStage::eVertex, vk::DescriptorType::eUniformBuffer, &m.cameraUniformBuffer }
        },
        .topology = vk::Pipeline::Topology::eTriangleFan,
        .cullMode = vk::Pipeline::CullMode::eNone,
        .depthWrite = true,
        .depthTest = false,
        .colorBlending = true
    }};

    m.imguiPipeline = vk::Pipeline{ m.device, vk::Pipeline::Config{
        .point = vk::Pipeline::BindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .path = "shaders/imgui.vert.spv" },
            { .stage = vk::ShaderStage::eFragment, .path = "shaders/imgui.frag.spv" }
        },
        .descriptors = {
            { 0, vk::ShaderStage::eVertex, vk::DescriptorType::eStorageBuffer, &m.imguiVertexBuffer },
            { 1, vk::ShaderStage::eVertex, vk::DescriptorType::eStorageBuffer, &m.imguiDrawBuffer },
            { 2, vk::ShaderStage::eFragment, vk::DescriptorType::eCombinedImageSampler }
        },
        .topology = vk::Pipeline::Topology::eTriangleList,
        .cullMode = vk::Pipeline::CullMode::eNone,
        .colorBlending = true
    }};

    m.imguiPipeline.writeImage(m.imguiFontTexture, 0, vk::DescriptorType::eCombinedImageSampler);

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

auto Renderer::initImgui() -> void
{
    ImGui::CreateContext();

    auto& io{ ImGui::GetIO() }; (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL3_InitForVulkan(const_cast<SDL_Window*>(m.window.getHandle()));
}

auto Renderer::terminateImgui() -> void
{
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
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

        m.indirectCommands.emplace_back(vk::DrawIndirectCommand{
            .vertexCount = mesh.indexCount,
            .instanceCount = 1,
            .firstVertex = mesh.indexOffset,
            .firstInstance = instance
        });
    }
}
