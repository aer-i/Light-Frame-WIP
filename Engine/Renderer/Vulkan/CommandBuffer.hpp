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
    class SwapBuffer;

    struct DrawIndirectCommand
    {
        u32 vertexCount;
        u32 instanceCount;
        u32 firstVertex;
        u32 firstInstance;
    };

    struct DrawIndexedIndirectCommand
    {
        u32 indexCount;
        u32 instanceCount;
        u32 firstIndex;
        i32 vertexOffset;
        u32 firstInstance;
    };

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
        auto begin(u32 frameIndex = 0) -> void;
        auto end() -> void;
        auto beginPresent() -> void;
        auto endPresent() -> void;
        auto pushConstant(void const* pData, size_t dataSize) -> void;
        auto beginRendering(Image const& image, Image const* pDepthImage = nullptr) -> void;
        auto endRendering() -> void;
        auto copyBuffer(Buffer& source, Buffer& destination, size_t size) -> void;
        auto barrier(Image& image, ImageLayout layout) -> void;
        auto bindIndexBuffer16(Buffer& indexBuffer) -> void;
        auto bindIndexBuffer16(SwapBuffer& indexBuffer) -> void;
        auto bindIndexBuffer32(Buffer& indexBuffer) -> void;
        auto bindIndexBuffer32(SwapBuffer& indexBuffer) -> void;
        auto bindPipeline(Pipeline& pipeline) -> void;
        auto draw(u32 vertexCount) -> void;
        auto drawIndexed(u32 indexCount, u32 indexOffset = 0, i32 vertexOffset = 0) -> void;
        auto drawIndirect(Buffer& buffer, u32 drawCount) -> void;
        auto drawIndexedIndirectCount(Buffer& buffer, u32 maxDraws) -> void;
        auto drawIndexedIndirectCount(SwapBuffer& buffer, u32 maxDraws) -> void;
        auto allocate(Device* pDevice) -> void;

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
            u32             frameIndex;
        } m;
    };
}