#include "Window.hpp"
#include <stdexcept>
#include <aixlog.hpp>

auto Window::Create() -> void
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_handle = glfwCreateWindow(m_size.x, m_size.y, m_title.c_str(), nullptr, nullptr);

    if (!m_handle)
    {
        throw std::runtime_error("Failed to create window");
    }
    
    glfwSetWindowPos(m_handle, m_pos.x, m_pos.y);

    glfwSetFramebufferSizeCallback(m_handle, Window::FramebufferResizeCallback);
    glfwSetWindowPosCallback(m_handle, Window::PositionChangeCallback);
    glfwSetInputMode(m_handle, GLFW_STICKY_KEYS, GLFW_TRUE);

    LOG(INFO, "Window") << "Created window\n";
}

auto Window::Teardown() -> void
{
    glfwDestroyWindow(m_handle);
    glfwTerminate();
    LOG(INFO, "Window") << "Destroyed window\n";
}

auto Window::Update() -> void
{
    auto static previousTime{ glfwGetTime() };
    auto        currentTime { glfwGetTime() };

    m_deltaTime =  static_cast<f32>(currentTime - previousTime);
    m_available = !static_cast<bool>(glfwWindowShouldClose(m_handle));

    glfwPollEvents();

    previousTime = currentTime;
}

auto Window::IsKeyPressed(i32 key) -> bool
{
    return glfwGetKey(m_handle, key);
}

auto Window::IsKeyDown(i32 key) -> bool
{
    return glfwGetKey(m_handle, key) == GLFW_PRESS;
}

auto Window::IsKeyUp(i32 key) -> bool
{
    return glfwGetKey(m_handle, key) == GLFW_RELEASE;
}

auto Window::SetTitle(std::string_view title) -> void
{
    m_title = title;
    glfwSetWindowTitle(m_handle, m_title.c_str());
}

auto Window::FramebufferResizeCallback(GLFWwindow* window, i32 width, i32 height) -> void
{
    m_size.x = width;
    m_size.y = height;
}

auto Window::PositionChangeCallback(GLFWwindow* window, i32 x, i32 y) -> void
{
    m_pos.x = x;
    m_pos.y = y;
}