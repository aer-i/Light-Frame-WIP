#pragma once
#include "Types.hpp"
#include "VulkanEnums.hpp"
#include <glm/glm.hpp>

struct VkCommandPool_T;
struct VkCommandBuffer_T;

using VkCommandPool   = VkCommandPool_T*;
using VkCommandBuffer = VkCommandBuffer_T*;

namespace vk
{
    class Device;
    class Image;
    class Pipeline;
    class Buffer;

    class CommandBuffer
    {
    public:
        CommandBuffer();
        ~CommandBuffer();
        CommandBuffer(CommandBuffer const&) = delete;
        CommandBuffer(CommandBuffer&& other);
        auto operator=(CommandBuffer const&) -> CommandBuffer& = delete;
        auto operator=(CommandBuffer&& other) -> CommandBuffer&;

    public:
        auto begin() -> void;
        auto end() -> void;
        auto beginPresent() -> void;
        auto endPresent() -> void;
        auto beginRendering(Image const& image) -> void;
        auto endRendering() -> void;
        auto pushConstant(const void* data, size_t size) -> void;
        auto copyBuffer(Buffer& source, Buffer& destination, size_t size) -> void;
        auto barrier(Image& image, ImageLayout layout) -> void;
        auto setScissor(glm::ivec2 offset, glm::uvec2 size) -> void;
        auto bindIndexBuffer16(Buffer& indexBuffer) -> void;
        auto bindIndexBuffer32(Buffer& indexBuffer) -> void;
        auto bindPipeline(Pipeline& pipeline) -> void;
        auto draw(u32 vertexCount) -> void;
        auto drawIndexed(u32 indexCount, u32 indexOffset = 0, i32 vertexOffset = 0) -> void;
        auto allocate(Device* pDevice) -> void;
        auto allocateForTransfers(Device* pDevice) -> void;

    public:
        using Handle = VkCommandBuffer;

        inline operator Handle() noexcept
        {
            return m.buffer;
        }

    private:
        struct M
        {
            Device*         device;
            Pipeline*       currentPipeline;
            VkCommandPool   pool;
            VkCommandBuffer buffer;
        } m;
    };
}