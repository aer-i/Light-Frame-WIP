#pragma once
#include "Types.hpp"
#include "VulkanEnums.hpp"
#include "BufferResource.hpp"
#include <cstring>

struct VkBuffer_T;
struct VmaAllocation_T;

using VkBuffer = VkBuffer_T*;
using VmaAllocation = VmaAllocation_T*;

namespace vk
{
    class Device;

    class Buffer
    {
    public:
        Buffer();
        Buffer(Device& device, u32 size, BufferUsageFlags usage, MemoryType memoryType);
        ~Buffer();
        Buffer(Buffer const&) = delete;
        Buffer(Buffer&& other);
        auto operator=(Buffer const&)  -> Buffer& = delete;
        auto operator=(Buffer&& other) -> Buffer&;

    public:
        auto write(void const* data, size_t size) -> void;
        auto write(void const* data, size_t size, size_t offset) -> void;
        auto flush(size_t size) -> void;

    public:
        inline operator VkBuffer() const noexcept
        {
            return m.buffer;
        }

        template<typename T = size_t>
        inline auto getSize() -> T
        {
            return static_cast<T>(m.size);
        }

    private:
        struct M
        {
            Device*       device;
            VkBuffer      buffer;
            VmaAllocation allocation;
            u8*           mappedData;
            u32           memoryType;
            u32           size;
        } m;
    };

    class SwapBuffer
    {
    public:
        SwapBuffer();
        SwapBuffer(Device& device, u32 size, BufferUsageFlags usage, MemoryType memoryType);
        ~SwapBuffer();
        SwapBuffer(SwapBuffer const&) = delete;
        SwapBuffer(SwapBuffer&& other);
        auto operator=(SwapBuffer const&)  -> SwapBuffer& = delete;
        auto operator=(SwapBuffer&& other) -> SwapBuffer&;

    public:
        auto write(void const* data, size_t size) -> void;
        auto write(void const* data, size_t size, size_t offset) -> void;
        auto flush(size_t size) -> void;

    public:
        template<typename T>
        inline auto operator()(T frameIndex) const noexcept -> VkBuffer
        {
            return m.frames[frameIndex].buffer;
        }

        template<typename T = size_t>
        inline auto getSize() -> T
        {
            return static_cast<T>(m.size);
        }

    private:
        struct M
        {
            struct Frame
            {
                VkBuffer buffer;
                VmaAllocation allocation;
                u8* mappedData;
            };
            
            std::pmr::vector<Frame> frames;
            
            Device* device;
            u32     memoryType;
            u32     size;
        } m;
    };
}