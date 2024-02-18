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
    vk::Buffer uvsBuffer;
    vk::Buffer indexBuffer;
    vk::Buffer uniformBuffer;
    vk::Image  texture;

    Camera camera;
    camera.yawPitch.x = -90.f;
    camera.position = { 0.f, 0.f, -3.f };
    camera.setProjection(glm::radians(70.f), vk::getAspectRatio(), 0.1f, 1024.f);
    camera.update();

    f32 vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };

    f32 uvs[] = {
        1.f, 0.f,
        0.f, 0.f,
        0.f, 1.f,
        1.f, 1.f
    };

    u32 indices[] = { 0, 1, 2, 0, 2, 3 };

    positionBuffer.allocate(sizeof(vertices),  vk::BufferUsage::eStorageBuffer, vk::MemoryType::eDevice);
    uvsBuffer.allocate     (sizeof(uvs),       vk::BufferUsage::eStorageBuffer, vk::MemoryType::eDevice);
    indexBuffer.allocate   (sizeof(indices),   vk::BufferUsage::eStorageBuffer, vk::MemoryType::eDevice);
    uniformBuffer.allocate (sizeof(glm::mat4), vk::BufferUsage::eUniformBuffer, vk::MemoryType::eHost);
    
    positionBuffer.writeData(vertices);
    uvsBuffer.writeData(uvs);
    indexBuffer.writeData(indices);

    texture.loadFromFile2D("rock.png");

    auto pipeline{ vk::createPipeline(vk::PipelineConfig{
        .bindPoint = vk::PipelineBindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .filepath = "shaders/main.vert.spv"},
            { .stage = vk::ShaderStage::eFragment, .filepath = "shaders/main.frag.spv"}
        },
        .descriptors = {
            { .binding = 0, .shaderStage = vk::ShaderStage::eVertex,   .descriptorType = vk::DescriptorType::eStorageBuffer,        .pBuffer = &indexBuffer    },
            { .binding = 1, .shaderStage = vk::ShaderStage::eVertex,   .descriptorType = vk::DescriptorType::eStorageBuffer,        .pBuffer = &positionBuffer },
            { .binding = 2, .shaderStage = vk::ShaderStage::eVertex,   .descriptorType = vk::DescriptorType::eStorageBuffer,        .pBuffer = &uvsBuffer      },
            { .binding = 3, .shaderStage = vk::ShaderStage::eVertex,   .descriptorType = vk::DescriptorType::eUniformBuffer,        .pBuffer = &uniformBuffer  },
            { .binding = 4, .shaderStage = vk::ShaderStage::eFragment, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pBuffer = nullptr         }
        }
    })};

    pipeline.writeImage(&texture, 0, vk::DescriptorType::eCombinedImageSampler);

    auto threadPool{ ThreadPool{vk::getCommandBufferCount()} };
    auto recordCommands{ [&]() -> void
    {
        for (auto i{ vk::getCommandBufferCount() }; i--; )
        {
            threadPool.enqueue([i, &pipeline]
            {
                auto cmd{ vk::Cmd{i} };
                cmd.begin();
                cmd.beginPresent();
                cmd.bindPipeline(pipeline);
                cmd.draw(6);
                cmd.endPresent();
                cmd.end();
            });
        }

        threadPool.wait();
    }};
    recordCommands();

    vk::onResize([&]() -> void
    {
        recordCommands();
        camera.setProjection(glm::radians(70.f), vk::getAspectRatio(), 0.1f, 1024.f);
    });

    auto previousTime{ Window::GetTime() };
    auto frameCount{ u32{} };
    auto relativeMouseMode{ bool{} };

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

        if (Window::GetKeyDown(key::eEscape))
        {
            relativeMouseMode = !relativeMouseMode;
            Window::SetRelativeMouseMode(relativeMouseMode);
        }
        
        Window::Update();

        if (Window::GetRelativeMouseMode())
        {
            camera.update();
        }

        auto projView{ camera.projection * camera.view };
        uniformBuffer.writeData(&projView);

        vk::present();
    }
    vk::waitIdle();
}

auto Engine::Teardown() -> void
{
    vk::teardown();
    Window::Teardown();
}