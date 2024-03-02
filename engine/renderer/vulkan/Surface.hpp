#pragma once

class Window;
struct VkSurfaceKHR_T;

using VkSurfaceKHR = VkSurfaceKHR_T*;

namespace vk
{
    class Instance;

    class Surface
    {
    public:
        Surface() = default;
        Surface(Window& window, Instance& instance);
        ~Surface();
        Surface(Surface&& other);
        Surface(Surface const&) = delete;
        auto operator=(Surface&& other) -> Surface&;
        auto operator=(Surface const&)  -> Surface& = delete;

    public:
        inline operator VkSurfaceKHR() const noexcept
        {
            return m_surface;
        }

    private:
        Instance* m_instance;
        VkSurfaceKHR m_surface;
    };
}