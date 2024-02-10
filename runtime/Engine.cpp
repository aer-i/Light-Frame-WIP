#include "Engine.hpp"
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
    auto pipeline{ vk::createPipeline(vk::PipelineConfig{
        .bindPoint = vk::PipelineBindPoint::eGraphics,
        .stages = {
            { .stage = vk::ShaderStage::eVertex,   .filepath = "shaders/main.vert.spv"},
            { .stage = vk::ShaderStage::eFragment, .filepath = "shaders/main.frag.spv"}
        }
    })};


    auto recordCommands{ [&]() -> void
    {
        for (auto i{ vk::getCommandBufferCount() }; i--; )
        {
            vk::cmdBegin();
            vk::cmdBeginPresent();
            vk::cmdBindPipeline(pipeline);
            vk::cmdDraw(3);
            vk::cmdEndPresent();
            vk::cmdEnd();
            vk::cmdNext();
        }
    }};

    recordCommands();
    vk::onResize(recordCommands);

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
        vk::present();
    }
}

auto Engine::Teardown() -> void
{
    vk::teardown();
    Window::Teardown();
}