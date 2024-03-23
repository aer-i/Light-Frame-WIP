#pragma once
#include "Image.hpp"
#include "CommandBuffer.hpp"
#include "BufferResource.hpp"
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
        enum class SwapchainResult
        {
            eSuccess    = 0,
            eRecreated  = 1,
            eTerminated = 2
        };

    public:
        Device(Instance& instance, Surface& surface, PhysicalDevice& physicalDevice);
        ~Device();
        Device(Device&& other);
        Device(Device const&) = delete;
        auto operator=(Device&& other) -> Device&;
        auto operator=(Device const&)  -> Device& = delete;

    public:
        auto waitIdle() -> void;
        auto checkSwapchainState(Window& window) -> SwapchainResult;
        auto submitAndPresent() -> void;
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

        inline auto getExtent() const noexcept -> glm::uvec2
        {
            return m.swapchainExtent;
        }

        inline auto getCommandBuffers() noexcept -> std::pmr::vector<CommandBuffer>&
        {
            return m.commandBuffers;
        }

        inline auto getSwapchainImage(u32 imageIndex) noexcept -> Image&
        {
            return m.swapchainImages[imageIndex];
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
            glm::uvec2       swapchainExtent;
            u32              imageIndex;
            u32              frameIndex;

            std::pmr::vector<Image>         swapchainImages;
            std::pmr::vector<CommandBuffer> commandBuffers;
            std::pmr::vector<VkSemaphore>   presentSemaphores;
            std::pmr::vector<VkSemaphore>   renderSemaphores;
            std::pmr::vector<VkFence>       fences;
        } m;  
    };
}