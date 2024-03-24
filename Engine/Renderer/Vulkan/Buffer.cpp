#include "Buffer.hpp"
#include "Device.hpp"
#include <volk.h>
#include <vk_mem_alloc.h>
#include <stdexcept>

vk::Buffer::Buffer()
    : m{}
{}

vk::Buffer::Buffer(Device& device, u32 size, BufferUsageFlags usage, MemoryType memoryType)
    : m{
        .device = &device,
        .size = size
    }
{
    auto bufferCreateInfo{ VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = m.size,
        .usage = usage
    }};

    auto allocationCreateInfo{ VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_AUTO
    }};

    switch (memoryType)
    {
    case MemoryType::eHost:
        allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        allocationCreateInfo.flags  = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        break;
    case MemoryType::eHostOnly:
        allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    case MemoryType::eDevice:
        allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        bufferCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        break;
    default:
        break;
    }

    auto allocationInfo{ VmaAllocationInfo{} };
    
    if (vmaCreateBuffer(*m.device, &bufferCreateInfo, &allocationCreateInfo, &m.buffer, &m.allocation, &allocationInfo))
    {
        throw std::runtime_error("Failed to allocate buffer");
    }

    m.mappedData = static_cast<u8*>(allocationInfo.pMappedData);

    vmaGetAllocationMemoryProperties(*m.device, m.allocation, &m.memoryType);
}

vk::Buffer::~Buffer()
{
    if (m.device && m.buffer && m.allocation)
    {
        vmaDestroyBuffer(*m.device, m.buffer, m.allocation);
    }
    
    m = {};
}

vk::Buffer::Buffer(Buffer&& other)
    : m{ std::move(other.m) }
{
    other.m = {};
}

auto vk::Buffer::operator=(Buffer&& other) -> Buffer&
{
    m = std::move(other.m);
    other.m = {};

    return *this;
}

auto vk::Buffer::write(void const* data, size_t size) -> void
{
    if (m.memoryType & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        std::memcpy(m.mappedData, data, size);
        vmaFlushAllocation(*m.device, m.allocation, 0, size);
    }
    else
    {
        auto const bufferCreateInfo{ VkBufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        }};

        auto const allocationCreateInfo{ VmaAllocationCreateInfo{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                     VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        }};

        auto stagingBuffer{ VkBuffer{} };
        auto stagingAllocation{ VmaAllocation{} };
        auto allocationInfo{ VmaAllocationInfo{} };

        if (vmaCreateBuffer(*m.device, &bufferCreateInfo, &allocationCreateInfo, &stagingBuffer, &stagingAllocation, &allocationInfo))
        {
            throw std::runtime_error("Failed to allocate buffer");
        }

        memcpy(allocationInfo.pMappedData, data, size);
        vmaFlushAllocation(*m.device, stagingAllocation, 0, size);

        m.device->transferSubmit([&](CommandBuffer& command)
        {
            auto const copy{ VkBufferCopy{
                .size = size
            }};

            vkCmdCopyBuffer(command, stagingBuffer, m.buffer, 1, &copy);
        });

        vmaDestroyBuffer(*m.device, stagingBuffer, stagingAllocation);
    }
}

auto vk::Buffer::write(void const* data, size_t size, size_t offset) -> void
{
    if (m.memoryType & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        std::memcpy(m.mappedData + offset, data, size);
    }
    else
    {
        auto const bufferCreateInfo{ VkBufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        }};

        auto const allocationCreateInfo{ VmaAllocationCreateInfo{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                     VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        }};

        auto stagingBuffer{ VkBuffer{} };
        auto stagingAllocation{ VmaAllocation{} };
        auto allocationInfo{ VmaAllocationInfo{} };

        if (vmaCreateBuffer(*m.device, &bufferCreateInfo, &allocationCreateInfo, &stagingBuffer, &stagingAllocation, &allocationInfo))
        {
            throw std::runtime_error("Failed to allocate buffer");
        }

        memcpy(allocationInfo.pMappedData, data, size);
        vmaFlushAllocation(*m.device, stagingAllocation, 0, size);

        m.device->transferSubmit([&](CommandBuffer& command)
        {
            auto const copy{ VkBufferCopy{
                .dstOffset = offset,
                .size = size
            }};

            vkCmdCopyBuffer(command, stagingBuffer, m.buffer, 1, &copy);
        });

        vmaDestroyBuffer(*m.device, stagingBuffer, stagingAllocation);
    }
}

auto vk::Buffer::flush(size_t size) -> void
{
    if (m.memoryType & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        vmaFlushAllocation(*m.device, m.allocation, 0, size);
    }
}