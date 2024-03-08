#pragma once
#include "Types.hpp"
#include "VulkanEnums.hpp"

class Window;
struct VkSurfaceKHR_T;

using VkSurfaceKHR = VkSurfaceKHR_T*;

namespace vk
{
    class Instance;
    class PhysicalDevice;

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
        struct Extent
        {
            u32 width, height;
        };

    public:
        auto getFormat(PhysicalDevice& physicalDevice) -> Format;
        auto presentModeSupport(PhysicalDevice& physicalDevice, PresentMode mode) -> bool;
        auto getExtent(PhysicalDevice& physicalDevice) -> Extent;
        auto getClampedImageCount(PhysicalDevice& physicalDevice, u32 imageCount) -> u32;

    public:
        inline operator VkSurfaceKHR() const noexcept
        {
            return m.surface;
        }

    private:
        struct M
        {
            Window*      window;
            Instance*    instance;
            VkSurfaceKHR surface;
        } m;
    };
}