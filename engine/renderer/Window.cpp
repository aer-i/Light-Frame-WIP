#include "Window.hpp"
#include <stdexcept>
#include <chrono>
#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <aixlog.hpp>

auto Window::Create() -> void
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
    {
        throw std::runtime_error("Failed to init SDL");
    }

    m_handle = SDL_CreateWindow(
        m_title.c_str(),
        static_cast<i32>(m_size.x),
        static_cast<i32>(m_size.y),
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );

    if (!m_handle)
    {
        throw std::runtime_error("Failed to create window");
    }
    
    SDL_SetWindowPosition(m_handle, m_pos.x, m_pos.y);
    m_keyboardState = const_cast<u8*>(SDL_GetKeyboardState(nullptr));

    LOG(INFO, "Window") << "Created window\n";
}

auto Window::Teardown() -> void
{
    SDL_DestroyWindow(m_handle);
    SDL_Quit();
    LOG(INFO, "Window") << "Destroyed window\n";
}

auto Window::Update() -> void
{
    auto static event       { SDL_Event{} };
    auto static previousTime{ GetTime()   };
    auto        currentTime { GetTime()   };

    while (SDL_PollEvent(&event))
    {
        if (ImGui::GetCurrentContext())
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
        }

        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            m_available = false;
            break;
        case SDL_EVENT_WINDOW_MINIMIZED:
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_RESTORED:
            SDL_GetWindowSize(m_handle, &m_size.x, &m_size.y);
            break;
        case SDL_EVENT_MOUSE_MOTION:
            SDL_GetMouseState(&m_cursorPos.x, &m_cursorPos.y);
            SDL_GetGlobalMouseState(&m_globCursorPos.x, &m_globCursorPos.y);
            break;
        default:
            break;
        }
    }

    SDL_GetRelativeMouseState(&m_cursorOff.x, &m_cursorOff.y);

    m_deltaTime = static_cast<f32>(currentTime - previousTime);
    previousTime = currentTime;

    m_keyboardState = const_cast<u8*>(SDL_GetKeyboardState(nullptr));
}

auto Window::SetTitle(std::string_view title) -> void
{
    m_title = title;
    SDL_SetWindowTitle(m_handle, m_title.c_str());
}

auto Window::SetRelativeMouseMode(bool enable) -> void
{
    SDL_SetRelativeMouseMode(enable);
}

auto Window::GetRelativeMouseMode() -> bool
{
    return static_cast<bool>(SDL_GetRelativeMouseMode());
}

auto Window::GetKey(i32 key) -> bool
{
    return m_keyboardState[key];
}

auto Window::GetKeyDown(i32 key) -> bool
{
    bool static prevKeyboardState[SDL_NUM_SCANCODES] = { false };

    if (m_keyboardState[key] && !prevKeyboardState[key])
    {
        prevKeyboardState[key] = true;
        return true;
    }
    if (!m_keyboardState[key])
    {
        prevKeyboardState[key] = false;
    }

    return false;
}

auto Window::GetKeyUp(i32 key) -> bool
{
    bool static prevKeyboardState[SDL_NUM_SCANCODES] = { false };

    if (!m_keyboardState[key] && prevKeyboardState[key])
    {
        prevKeyboardState[key] = false;
        return true;
    }
    if (m_keyboardState[key])
    {
        prevKeyboardState[key] = true;
    }

    return false;
}

auto Window::GetButton(int button) -> bool
{
    return false;
}

auto Window::GetButtonDown(int button) -> bool
{
    return false;
}

auto Window::GetButtonUp(int button) -> bool
{
    return false;
}

auto Window::GetTime() -> f64
{
    auto static startTime{ std::chrono::high_resolution_clock::now() };
    auto now{ std::chrono::high_resolution_clock::now() };
    return std::chrono::duration_cast<std::chrono::duration<double>>(now - startTime).count();
}