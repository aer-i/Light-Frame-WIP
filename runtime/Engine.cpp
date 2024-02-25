#include "Engine.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include "ImguiRenderer.hpp"
#include "Grid.hpp"
#include "Thread.hpp"
#include "Scene.hpp"
#include "TextRenderer.hpp"
#include <aixlog.hpp>

auto Engine::Init() -> void
{
    AixLog::Log::init<AixLog::SinkCout>(AixLog::Severity::trace);
    Window::Create();
    vk::init();
    vk::imgui::init();
}

auto Engine::Execute() -> void
{
    vk::Buffer positionBuffer;
    vk::Buffer uvsBuffer;
    vk::Buffer indexBuffer;
    vk::Buffer uniformBuffer;
    vk::Image  texture;
    Font     font("assets/fonts/Arial.ttf");
    TextRenderer textRenderer(font);

    auto scene{ Scene{"Main_scene"} };

    Camera camera;
    camera.yawPitch.x = -90.f;
    camera.position = { 0.f, 0.f, -3.f };
    camera.setProjection(glm::radians(40.f), vk::getAspectRatio(), 0.1f, 1024.f);
    camera.update();

    positionBuffer.allocate(scene.m_meshLoader.m_positions.size() * 4,  vk::BufferUsage::eStorageBuffer, vk::MemoryType::eDevice);
    uvsBuffer.allocate     (scene.m_meshLoader.m_uvs.size() * 4,       vk::BufferUsage::eStorageBuffer, vk::MemoryType::eDevice);
    indexBuffer.allocate   (scene.m_meshLoader.m_indices.size() * 4,   vk::BufferUsage::eStorageBuffer, vk::MemoryType::eDevice);
    uniformBuffer.allocate (sizeof(glm::mat4) * 3, vk::BufferUsage::eUniformBuffer, vk::MemoryType::eHost);
    
    positionBuffer.writeData(scene.m_meshLoader.m_positions.data());
    uvsBuffer.writeData(scene.m_meshLoader.m_uvs.data());
    indexBuffer.writeData(scene.m_meshLoader.m_indices.data());

    texture.loadFromFile2D("assets/textures/rock.png");

    auto pipeline{ vk::createPipeline(vk::PipelineConfig{
        .bindPoint = vk::PipelineBindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .path = "shaders/main.vert.spv"},
            { .stage = vk::ShaderStage::eFragment, .path = "shaders/main.frag.spv"}
        },
        .descriptors = {
            { .binding = 0, .shaderStage = vk::ShaderStage::eVertex,   .descriptorType = vk::DescriptorType::eStorageBuffer,        .pBuffer = &indexBuffer    },
            { .binding = 1, .shaderStage = vk::ShaderStage::eVertex,   .descriptorType = vk::DescriptorType::eStorageBuffer,        .pBuffer = &positionBuffer },
            { .binding = 2, .shaderStage = vk::ShaderStage::eVertex,   .descriptorType = vk::DescriptorType::eStorageBuffer,        .pBuffer = &uvsBuffer      },
            { .binding = 3, .shaderStage = vk::ShaderStage::eVertex,   .descriptorType = vk::DescriptorType::eUniformBuffer,        .pBuffer = &uniformBuffer  },
            { .binding = 4, .shaderStage = vk::ShaderStage::eFragment, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pBuffer = nullptr         }
        },
        .topology = vk::PipelineTopology::eTriangleList
    })};

    pipeline.writeImage(&texture, 0, vk::DescriptorType::eCombinedImageSampler);
    auto grid{ GridRenderer{uniformBuffer} };

    auto recordCommands{ [&]() -> void
    {
        for (auto i{ vk::getCommandBufferCount() }; i--; )
        {
            auto cmd{ vk::Cmd{i} };
            cmd.begin();
            cmd.beginPresent();
            grid.draw(cmd);
            cmd.bindPipeline(pipeline);
            cmd.draw(86832);
            textRenderer.recordCommands(cmd);
            vk::imgui::render(cmd);
            cmd.endPresent();
            cmd.end();
        }
    }};
    recordCommands();

    vk::onRecord(recordCommands);
    vk::onResize([&]() -> void
    {
        recordCommands();
        camera.setProjection(glm::radians(70.f), vk::getAspectRatio(), 0.1f, 1024.f);
    });


    auto previousTime{ Window::GetTime() };
    auto frameCount{ u32{} };
    auto relativeMouseMode{ bool{} };
    auto fpsText{ std::string{} };

    while (Window::Available())
    {
        auto currentTime{ Window::GetTime() };
        ++frameCount;

        if (currentTime - previousTime >= 1.0)
        {
            fpsText = "fps: " + std::to_string(frameCount);
            frameCount = 0;
            previousTime = currentTime;
        }

        Window::Update();

        if (Window::GetKeyDown(key::eEscape))
        {
            relativeMouseMode = !relativeMouseMode;
            Window::SetRelativeMouseMode(relativeMouseMode);
        }

        if (Window::GetRelativeMouseMode())
        {
            camera.update();
        }

        glm::mat4 uniformData[3] = {
            camera.projection,
            camera.view,
            camera.projection * camera.view
        };

        uniformBuffer.writeData(uniformData);

        textRenderer.render2D(fpsText, { Window::GetWidth() - 300.f, Window::GetHeight() - 50.f}, 0.2f);
        textRenderer.render2D("Hello world", {50, 50});
        textRenderer.flush();

        ImGui::ShowDemoWindow();
        
        vk::imgui::update();
        vk::present();
    }
    vk::waitIdle();
}

auto Engine::Teardown() -> void
{
    vk::imgui::teardown();
    vk::teardown();
    Window::Teardown();
}