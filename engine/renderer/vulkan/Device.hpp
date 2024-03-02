#pragma once

struct VkDevice_T;
struct VkQueue_T;
struct VkSwapchainKHR_T;
struct VmaAllocator_T;

using VkDevice       = VkDevice_T*;
using VkQueue        = VkQueue_T*;
using VkSwapchainKHR = VkSwapchainKHR_T*;
using VmaAllocator   = VmaAllocator_T*;

namespace vk
{
    class Instance;
    class Surface;
    class PhysicalDevice;

    class Device
    {
    public:
        Device(Instance& instance, Surface& surface, PhysicalDevice& physicalDevice);
        ~Device();
        Device(Device&& other);
        Device(Device const&) = delete;
        auto operator=(Device&& other) -> Device&;
        auto operator=(Device const&)  -> Device& = delete;

    public:
        inline operator VkDevice() const noexcept
        {
            return m_device;
        }

    private:
        auto createDevice(Instance& instance)    -> void;
        auto createAllocator(Instance& instance) -> void;
        auto createSwapchain()                   -> void;

    private:
        Surface*        m_surface;
        PhysicalDevice* m_physicalDevice;
        VkDevice        m_device;
        VkQueue         m_queue;
        VkSwapchainKHR  m_swapchain;
        VmaAllocator    m_allocator;
    };
}