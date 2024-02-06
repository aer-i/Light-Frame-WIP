#include "Engine.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include <aixlog.hpp>

auto Engine::Init() -> void
{
    AixLog::Log::init<AixLog::SinkCout>(AixLog::Severity::trace);
    Window::Create();
    vk::init();
}

auto Engine::Execute() -> void
{
    while (Window::Available())
    {
        Window::Update();
    }
}

auto Engine::Teardown() -> void
{
    vk::teardown();
    Window::Teardown();
}