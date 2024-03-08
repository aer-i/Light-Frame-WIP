#pragma once
#include "Image.hpp"
#include "CommandBuffer.hpp"
#include "ArrayProxy.hpp"
#include <functional>

class Window;

struct VkDevice_T;
struct VkQueue_T;
struct VkSwapchainKHR_T;
struct VkSemaphore_T;
struct VkFence_T;
struct VkDescriptorPool_T;
struct VkSampler_T;
struct VmaAllocator_T;

using VkDevice         = VkDevice_T*;
using VkQueue          = VkQueue_T*;
using VkSwapchainKHR   = VkSwapchainKHR_T*;
using VkSemaphore      = VkSemaphore_T*;
using VkFence          = VkFence_T*;
using VkDescriptorPool = VkDescriptorPool_T*;
using VkSampler        = VkSampler_T*;
using VmaAllocator     = VmaAllocator_T*;

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
        auto transferSubmit(std::function<void(CommandBuffer&)>&& function) -> void;

    public:
        inline operator VkDevice() const noexcept
        {
            return m.device;
        }

        inline operator VmaAllocator() const noexcept
        {
            return m.allocator;
        }

        inline operator VkDescriptorPool() const noexcept
        {
            return m.descriptorPool;
        }

        inline operator VkSampler() const noexcept
        {
            return m.sampler;
        }

        inline auto getCommandBuffer() noexcept -> CommandBuffer&
        {
            return m.commandBuffers[m.frameIndex];
        }

        inline auto getSwapchainImage() noexcept -> Image&
        {
            return m.swapchainImages[m.imageIndex];
        }

        inline auto getSurfaceFormat() noexcept -> Format
        {
            return m.surfaceFormat;
        }

    private:
        auto createDevice(Instance& instance)    -> void;
        auto createAllocator(Instance& instance) -> void;
        auto createSwapchain()                   -> void;
        auto createCommandBuffers()              -> void;
        auto createSyncObjects()                 -> void;
        auto createTransferResources()           -> void;
        auto createDescriptorPool()              -> void;
        auto createSampler()                     -> void;

    private:
        struct M
        {
            Surface*         surface;
            PhysicalDevice*  physicalDevice;
            VkDevice         device;
            VkQueue          queue;
            VkSwapchainKHR   swapchain;
            VkSwapchainKHR   oldSwapchain;
            VkDescriptorPool descriptorPool;
            VkSampler        sampler;
            VkFence          transferFence;
            CommandBuffer    transferCommandBuffer;
            VmaAllocator     allocator;
            Format           surfaceFormat;
            u32              imageIndex;
            u32              frameIndex;
            u32              imageCount;

            std::vector<VkSemaphore>   presentSemaphores;
            std::vector<VkSemaphore>   renderSemaphores;
            std::vector<VkFence>       fences;
            std::vector<Image>         swapchainImages;
            std::vector<CommandBuffer> commandBuffers;
        } m;  
    };
}