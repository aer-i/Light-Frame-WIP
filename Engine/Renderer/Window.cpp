#include "Window.hpp"
#include <stdexcept>
#include <chrono>
#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <spdlog/spdlog.h>

Window::Window()
    : m{
        .size = {1280, 720},
        .pos = {50, 50},
        .title = std::pmr::string("Light Frame", &pmr::g_rsrc),
        .available = true
    }
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
    {
        throw std::runtime_error("Failed to init SDL: " + std::string(SDL_GetError()));
    }

    spdlog::info("Initialized SDL content");

    if (!(m.handle = SDL_CreateWindow(
        m.title.c_str(),
        static_cast<i32>(m.size.x),
        static_cast<i32>(m.size.y),
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    )))
    {
        throw std::runtime_error("Failed to create window: " + std::string(SDL_GetError()));
    }
    
    SDL_SetWindowPosition(m.handle, m.pos.x, m.pos.y);
    m.keyboardState = const_cast<u8*>(SDL_GetKeyboardState(nullptr));

    SDL_GetWindowSize(m.handle, &m.size.x, &m.size.y);

    spdlog::info("Created window");
    spdlog::info("Window size [ {} width; {} height ]", m.size.x, m.size.y);
}

Window::~Window()
{
    SDL_DestroyWindow(m.handle);
    SDL_Quit();

    spdlog::info("Destroyed window");
    spdlog::info("Terminated SDL content");
}

auto Window::update() -> void
{
    auto static event       { SDL_Event{} };
    auto static previousTime{ getTime()   };
    auto        currentTime { getTime()   };

    while (SDL_PollEvent(&event))
    {
        if (ImGui::GetCurrentContext()) [[likely]]
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
        }

        switch (event.type)
        {
        [[unlikely]] case SDL_EVENT_QUIT:
            m.available = false;
            break;
        [[unlikely]] case SDL_EVENT_WINDOW_MINIMIZED:
            m.size = { 0, 0 };
            break;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_RESTORED:
            SDL_GetWindowSize(m.handle, &m.size.x, &m.size.y);
            break;
        [[likely]] case SDL_EVENT_MOUSE_MOTION:
            SDL_GetMouseState(&m.cursorPos.x, &m.cursorPos.y);
            SDL_GetGlobalMouseState(&m.globCursorPos.x, &m.globCursorPos.y);
            break;
        [[likely]] default:
            break;
        }
    }

    SDL_GetRelativeMouseState(&m.cursorOff.x, &m.cursorOff.y);

    m.deltaTime = static_cast<f32>(currentTime - previousTime);
    previousTime = currentTime;

    m.keyboardState = const_cast<u8*>(SDL_GetKeyboardState(nullptr));
}

auto Window::setTitle(std::string_view title) -> void
{
    m.title = title;
    SDL_SetWindowTitle(m.handle, m.title.c_str());
}

auto Window::setRelativeMouseMode(bool enable) -> void
{
    SDL_SetRelativeMouseMode(enable);
}

auto Window::getRelativeMouseMode() -> bool
{
    return static_cast<bool>(SDL_GetRelativeMouseMode());
}

auto Window::getKey(i32 key) -> bool
{
    return m.keyboardState[key];
}

auto Window::getKeyDown(i32 key) -> bool
{
    bool static prevKeyboardState[SDL_NUM_SCANCODES] = { false };

    if (m.keyboardState[key] && !prevKeyboardState[key])
    {
        prevKeyboardState[key] = true;
        return true;
    }

    if (!m.keyboardState[key])
    {
        prevKeyboardState[key] = false;
    }

    return false;
}

auto Window::getKeyUp(i32 key) -> bool
{
    bool static prevKeyboardState[SDL_NUM_SCANCODES] = { false };

    if (!m.keyboardState[key] && prevKeyboardState[key])
    {
        prevKeyboardState[key] = false;
        return true;
    }

    if (m.keyboardState[key])
    {
        prevKeyboardState[key] = true;
    }

    return false;
}

auto Window::getButton(i32 button) -> bool
{
    return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(button);
}

auto Window::getButtonDown(i32 button) -> bool
{
    bool static prevMouseState[6] = { false };
    auto mouseState{ SDL_GetMouseState(nullptr, nullptr) };
    auto pressed{ static_cast<bool>(mouseState & SDL_BUTTON(button)) };

    if (pressed && !prevMouseState[button])
    {
        prevMouseState[button] = true;
        return true;
    }

    if (!pressed)
    {
        prevMouseState[button] = false;
    }

    return false;
}

auto Window::getButtonUp(i32 button) -> bool
{
    bool static prevMouseState[6] = { false };
    auto mouseState{ SDL_GetMouseState(nullptr, nullptr) };
    auto pressed{ static_cast<bool>(mouseState & SDL_BUTTON(button)) };

    if (!pressed && prevMouseState[button])
    {
        prevMouseState[button] = false;
        return true;
    }

    if (pressed)
    {
        prevMouseState[button] = true;
    }

    return false;
}

auto Window::getTime() -> f64
{
    auto static startTime{ std::chrono::high_resolution_clock::now() };
    auto now{ std::chrono::high_resolution_clock::now() };
    return std::chrono::duration_cast<std::chrono::duration<double>>(now - startTime).count();
}