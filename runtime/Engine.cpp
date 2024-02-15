#include "Engine.hpp"
#include "Camera.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include "Thread.hpp"
#include <aixlog.hpp>

auto Engine::Init() -> void
{
    AixLog::Log::init<AixLog::SinkCout>(AixLog::Severity::trace);
    Window::Create();
    vk::init();
}

auto Engine::Execute() -> void
{
    vk::Buffer positionBuffer;
    vk::Buffer indexBuffer;
    vk::Buffer uniformBuffer;
    vk::Buffer colorBuffer;

    Camera camera;
    camera.position = { 0.f, 0.f, -3.f };
    camera.setProjection(glm::radians(70.f), 1280.f / 720.f, 0.1f, 1024.f);

    f32 vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };

    u32 indices[] = { 0, 1, 2, 0, 2, 3 };

    positionBuffer.allocate(sizeof(vertices),  vk::BufferUsage::eStorageBuffer, vk::MemoryType::eHost);
    indexBuffer.allocate   (sizeof(indices),   vk::BufferUsage::eStorageBuffer, vk::MemoryType::eHost);
    uniformBuffer.allocate (sizeof(glm::mat4), vk::BufferUsage::eUniformBuffer, vk::MemoryType::eHost);
    colorBuffer.allocate   (sizeof(f32) * 3,   vk::BufferUsage::eUniformBuffer, vk::MemoryType::eHost);
    
    positionBuffer.writeData(vertices);
    indexBuffer.writeData(indices);

    auto pipeline{ vk::createPipeline(vk::PipelineConfig{
        .bindPoint = vk::PipelineBindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .filepath = "shaders/main.vert.spv"},
            { .stage = vk::ShaderStage::eFragment, .filepath = "shaders/main.frag.spv"}
        },
        .descriptors = {
            { .binding = 0, .shaderStage = vk::ShaderStage::eVertex, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBuffer = &indexBuffer    },
            { .binding = 1, .shaderStage = vk::ShaderStage::eVertex, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBuffer = &positionBuffer },
            { .binding = 2, .shaderStage = vk::ShaderStage::eVertex, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBuffer = &uniformBuffer  },
            { .binding = 3, .shaderStage = vk::ShaderStage::eVertex, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBuffer = &colorBuffer    }
        }
    })};


    auto recordCommands{ [&]() -> void
    {
        for (auto i{ vk::getCommandBufferCount() }; i--; )
        {
            vk::cmdBegin();
            vk::cmdBeginPresent();
            vk::cmdBindPipeline(pipeline);
            vk::cmdDraw(6);
            vk::cmdEndPresent();
            vk::cmdEnd();
            vk::cmdNext();
        }
    }};
    recordCommands();

    vk::onResize([&]() -> void
    {
        recordCommands();
    });

    auto previousTime{ Window::GetTime() };
    auto frameCount{ u32{} };

    while (Window::Available())
    {
        auto currentTime{ Window::GetTime() };
        ++frameCount;

        if (currentTime - previousTime >= 1.0)
        {
            Window::SetTitle(std::string("Frames per second: ") + std::to_string(frameCount));
            frameCount = 0;
            previousTime = currentTime;
        }

        Window::Update();
        vk::acquire();

        camera.update();

        auto projView{ camera.projection * camera.view };
        uniformBuffer.writeData(&projView);
        
        f32 colors[] = { sinf((f32)Window::GetTime() * 10.f) * 0.5f + 0.5f, cosf((f32)Window::GetTime()) * 0.5f + 0.5f, sinf((f32)Window::GetTime()) * 0.5f + 0.5f };
        colorBuffer.writeData(colors);

        vk::present();
    }
    vk::waitIdle();
}

auto Engine::Teardown() -> void
{
    vk::teardown();
    Window::Teardown();
}