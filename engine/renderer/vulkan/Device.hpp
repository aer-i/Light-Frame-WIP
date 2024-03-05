#pragma once
#include "Image.hpp"
#include "CommandBuffer.hpp"
#include "ArrayProxy.hpp"

class Window;

struct VkDevice_T;
struct VkQueue_T;
struct VkSwapchainKHR_T;
struct VkSemaphore_T;
struct VkFence_T;
struct VmaAllocator_T;

using VkDevice       = VkDevice_T*;
using VkQueue        = VkQueue_T*;
using VkSwapchainKHR = VkSwapchainKHR_T*;
using VkSemaphore    = VkSemaphore_T*;
using VkFence        = VkFence_T*;
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
        auto waitIdle() -> void;
        auto checkSwapchainState(Window& window) -> void;
        auto acquireImage() -> void;
        auto submitCommands(ArrayProxy<CommandBuffer::Handle> const& commands) -> void;
        auto present() -> void;

    public:
        inline operator VkDevice() const noexcept
        {
            return m_device;
        }

        inline operator VmaAllocator() const noexcept
        {
            return m_allocator;
        }

        inline auto getCommandBuffer() noexcept -> CommandBuffer&
        {
            return m_commandBuffers[m_frameIndex];
        }

        inline auto getSwapchainImage() noexcept -> Image&
        {
            return m_swapchainImages[m_imageIndex];
        }

        inline auto getSurfaceFormat() noexcept -> Format
        {
            return m_surfaceFormat;
        }

    private:
        auto createDevice(Instance& instance)    -> void;
        auto createAllocator(Instance& instance) -> void;
        auto createSwapchain()                   -> void;
        auto createCommandBuffers()              -> void;
        auto createSyncObjects()                 -> void;

    private:
        Surface*        m_surface;
        PhysicalDevice* m_physicalDevice;
        VkDevice        m_device;
        VkQueue         m_queue;
        VkSwapchainKHR  m_swapchain;
        VkSwapchainKHR  m_oldSwapchain;
        VmaAllocator    m_allocator;
        Format          m_surfaceFormat;
        u32             m_imageIndex;
        u32             m_frameIndex;
        u32             m_imageCount;

        std::vector<VkSemaphore>   m_presentSemaphores;
        std::vector<VkSemaphore>   m_renderSemaphores;
        std::vector<VkFence>       m_fences;
        std::vector<Image>         m_swapchainImages;
        std::vector<CommandBuffer> m_commandBuffers;
    };
}