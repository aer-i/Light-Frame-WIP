#pragma once
#include "Types.hpp"
#include "VulkanEnums.hpp"

struct VkCommandPool_T;
struct VkCommandBuffer_T;

using VkCommandPool   = VkCommandPool_T*;
using VkCommandBuffer = VkCommandBuffer_T*;

namespace vk
{
    class Device;
    class Image;
    class Pipeline;

    class CommandBuffer
    {
    public:
        CommandBuffer();
        ~CommandBuffer();

    public:
        auto begin() -> void;
        auto end() -> void;
        auto beginPresent() -> void;
        auto endPresent() -> void;
        auto beginRendering(Image const& image) -> void;
        auto endRendering() -> void;
        auto barrier(Image& image, ImageLayout layout) -> void;
        auto bindPipeline(Pipeline& pipeline) -> void;
        auto draw(u32 vertexCount) -> void;
        auto allocate(Device* pDevice) -> void;

    public:
        using Handle = VkCommandBuffer;

        inline operator Handle() noexcept
        {
            return m_buffer;
        }

    private:
        Device*         m_device;
        VkCommandPool   m_pool;
        VkCommandBuffer m_buffer;
    };
}