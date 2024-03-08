#include "CommandBuffer.hpp"
#include "Device.hpp"
#include "Image.hpp"
#include "Buffer.hpp"
#include "Pipeline.hpp"
#include <volk.h>
#include <stdexcept>

vk::CommandBuffer::CommandBuffer()
    : m{}
{}

vk::CommandBuffer::~CommandBuffer()
{
    if (m.device && m.pool)
    {
        vkDestroyCommandPool(*m.device, m.pool, nullptr);
    }

    m = {};
}

vk::CommandBuffer::CommandBuffer(CommandBuffer&& other)
    : m{ std::move(other.m) }
{
    other.m = {};
}

auto vk::CommandBuffer::operator=(CommandBuffer&& other) -> CommandBuffer&
{
    m = std::move(other.m);
    other.m = {};

    return *this;
}

auto vk::CommandBuffer::begin() -> void
{
    if (vkResetCommandPool(*m.device, m.pool, 0))
    {
        throw std::runtime_error("Failed to reset VkCommandPool");
    }

    auto const beginInfo{ VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }};

    if (vkBeginCommandBuffer(m.buffer, &beginInfo))
    {
        throw std::runtime_error("Failed to begin VkCommandBuffer");
    }
}

auto vk::CommandBuffer::end() -> void
{
    if (vkEndCommandBuffer(m.buffer))
    {
        throw std::runtime_error("Failed to end VkCommandBuffer");
    }
}

auto vk::CommandBuffer::beginPresent() -> void
{
    barrier(m.device->getSwapchainImage(), ImageLayout::eColorAttachment);
    beginRendering(m.device->getSwapchainImage());
}

auto vk::CommandBuffer::endPresent() -> void
{
    endRendering();
    barrier(m.device->getSwapchainImage(), ImageLayout::ePresent);
}

auto vk::CommandBuffer::beginRendering(Image const& image) -> void
{
    {
        auto const colorAttachment{ VkRenderingAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = image,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE
        }};

        auto const renderingInfo{ VkRenderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {
                .extent = { 
                    .width  = image.getWidth(),
                    .height = image.getHeight()
                }
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment
        }};

        vkCmdBeginRendering(m.buffer, &renderingInfo);
    }
    {
        auto const viewport{ VkViewport{
            .y = static_cast<f32>(image.getHeight()),
            .width  =  static_cast<f32>(image.getWidth()),
            .height = -static_cast<f32>(image.getHeight()),
            .maxDepth = 1.f
        }};

        vkCmdSetViewport(m.buffer, 0, 1, &viewport);
    }
    {
        auto scissor{ VkRect2D{
            .extent = {
                .width  = image.getWidth(),
                .height = image.getHeight()
            }
        }};

        vkCmdSetScissor(m.buffer, 0, 1, &scissor);
    }
}

auto vk::CommandBuffer::endRendering() -> void
{
    vkCmdEndRendering(m.buffer);
}

auto vk::CommandBuffer::copyBuffer(Buffer& source, Buffer& destination, size_t size) -> void
{
    auto const copy{ VkBufferCopy{
        .size = size
    }};

    vkCmdCopyBuffer(m.buffer, source, destination, 1, &copy);
}

auto vk::CommandBuffer::barrier(Image& image, ImageLayout layout) -> void
{
    auto imageBarrier{ VkImageMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .oldLayout = static_cast<VkImageLayout>(image.getLayout()),
        .newLayout = static_cast<VkImageLayout>(layout),
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = image.getAspect(),
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .layerCount = VK_REMAINING_ARRAY_LAYERS
        },
    }};

    switch (image.getLayout())
    {
    case ImageLayout::eUndefined:
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        break;
    case ImageLayout::eColorAttachment:
        switch (layout)
        {
        case ImageLayout::ePresent:
            imageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_2_NONE;
            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        default:
            break;
        }
        break;
    case ImageLayout::ePresent:
        switch (layout)
        {
        case ImageLayout::eColorAttachment:
            imageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
            imageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    auto const dependency{ VkDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier
    }};

    vkCmdPipelineBarrier2(m.buffer, &dependency);
    image.setLayout(layout);
}

auto vk::CommandBuffer::setScissor(glm::ivec2 offset, glm::uvec2 size) -> void
{
    auto const scissor{ VkRect2D{
        .offset = {
            .x = offset.x,
            .y = offset.y
        },
        .extent = {
            .width = size.x,
            .height = size.y
        }
    }};

    vkCmdSetScissor(m.buffer, 0, 1, &scissor);
}

auto vk::CommandBuffer::bindIndexBuffer16(Buffer& indexBuffer) -> void
{
    vkCmdBindIndexBuffer(m.buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
}

auto vk::CommandBuffer::bindIndexBuffer32(Buffer& indexBuffer) -> void
{
    vkCmdBindIndexBuffer(m.buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

auto vk::CommandBuffer::bindPipeline(Pipeline& pipeline) -> void
{
    vkCmdBindPipeline(m.buffer, static_cast<VkPipelineBindPoint>(pipeline.getBindPoint()), pipeline);

    auto const set{ VkDescriptorSet{pipeline} };

    if (set)
    {
        vkCmdBindDescriptorSets(
            m.buffer,
            static_cast<VkPipelineBindPoint>(pipeline.getBindPoint()),
            pipeline,
            0,
            1,
            &set,
            0,
            nullptr
        );
    }
}

auto vk::CommandBuffer::draw(u32 vertexCount) -> void
{
    vkCmdDraw(m.buffer, vertexCount, 1, 0, 0);
}

auto vk::CommandBuffer::drawIndexed(u32 indexCount, u32 indexOffset, i32 vertexOffset) -> void
{
    vkCmdDrawIndexed(m.buffer, indexCount, 1, indexOffset, vertexOffset, 0);
}

auto vk::CommandBuffer::allocate(Device* pDevice) -> void
{
    m.device = pDevice;

    auto const commandPoolCreateInfo{ VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
    }};

    if (vkCreateCommandPool(*m.device, &commandPoolCreateInfo, nullptr, &m.pool))
    {
        throw std::runtime_error("Failed to create VkCommandPool");
    }

    auto const commandBufferAllocateInfo{ VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m.pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    }};

    if (vkAllocateCommandBuffers(*m.device, &commandBufferAllocateInfo, &m.buffer))
    {
        throw std::runtime_error("Failed to allocate VkCommandBuffer");
    }
}

auto vk::CommandBuffer::allocateForTransfers(Device* pDevice) -> void
{
    m.device = pDevice;

    auto const commandPoolCreateInfo{ VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
    }};

    if (vkCreateCommandPool(*m.device, &commandPoolCreateInfo, nullptr, &m.pool))
    {
        throw std::runtime_error("Failed to create VkCommandPool");
    }

    auto const commandBufferAllocateInfo{ VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m.pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    }};

    if (vkAllocateCommandBuffers(*m.device, &commandBufferAllocateInfo, &m.buffer))
    {
        throw std::runtime_error("Failed to allocate VkCommandBuffer");
    }
}
