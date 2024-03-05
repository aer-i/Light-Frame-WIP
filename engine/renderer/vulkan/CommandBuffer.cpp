#include "CommandBuffer.hpp"
#include "Device.hpp"
#include "Image.hpp"
#include "Pipeline.hpp"
#include <volk.h>
#include <stdexcept>

vk::CommandBuffer::CommandBuffer()
    : m_device{ nullptr }
    , m_pool{ nullptr }
    , m_buffer{ nullptr }
{}

vk::CommandBuffer::~CommandBuffer()
{
    if (m_device && m_pool)
    {
        vkDestroyCommandPool(*m_device, m_pool, nullptr);
    }
}

auto vk::CommandBuffer::begin() -> void
{
    if (vkResetCommandPool(*m_device, m_pool, 0))
    {
        throw std::runtime_error("Failed to reset VkCommandPool");
    }

    auto const beginInfo{ VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }};

    if (vkBeginCommandBuffer(m_buffer, &beginInfo))
    {
        throw std::runtime_error("Failed to begin VkCommandBuffer");
    }
}

auto vk::CommandBuffer::end() -> void
{
    if (vkEndCommandBuffer(m_buffer))
    {
        throw std::runtime_error("Failed to end VkCommandBuffer");
    }
}

auto vk::CommandBuffer::beginPresent() -> void
{
    barrier(m_device->getSwapchainImage(), ImageLayout::eColorAttachment);
    beginRendering(m_device->getSwapchainImage());
}

auto vk::CommandBuffer::endPresent() -> void
{
    endRendering();
    barrier(m_device->getSwapchainImage(), ImageLayout::ePresent);
}

auto vk::CommandBuffer::beginRendering(Image const& image) -> void
{
    {
        auto const colorAttachment{ VkRenderingAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = image,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {
                .color = VkClearColorValue{ 1.f, 1.f, 1.f, 1.f }
            }
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

        vkCmdBeginRendering(m_buffer, &renderingInfo);
    }
    {
        auto const viewport{ VkViewport{
            .y = static_cast<f32>(image.getHeight()),
            .width  =  static_cast<f32>(image.getWidth()),
            .height = -static_cast<f32>(image.getHeight()),
            .maxDepth = 1.f
        }};

        vkCmdSetViewport(m_buffer, 0, 1, &viewport);
    }
    {
        auto scissor{ VkRect2D{
            .extent = {
                .width  = image.getWidth(),
                .height = image.getHeight()
            }
        }};

        vkCmdSetScissor(m_buffer, 0, 1, &scissor);
    }
}

auto vk::CommandBuffer::endRendering() -> void
{
    vkCmdEndRendering(m_buffer);
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

    vkCmdPipelineBarrier2(m_buffer, &dependency);
    image.setLayout(layout);
}

auto vk::CommandBuffer::bindPipeline(Pipeline& pipeline) -> void
{
    vkCmdBindPipeline(m_buffer, static_cast<VkPipelineBindPoint>(pipeline.getBindPoint()), pipeline);
}

auto vk::CommandBuffer::draw(u32 vertexCount) -> void
{
    vkCmdDraw(m_buffer, vertexCount, 1, 0, 0);
}

auto vk::CommandBuffer::allocate(Device* pDevice) -> void
{
    m_device = pDevice;

    auto const commandPoolCreateInfo{ VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
    }};

    if (vkCreateCommandPool(*m_device, &commandPoolCreateInfo, nullptr, &m_pool))
    {
        throw std::runtime_error("Failed to create VkCommandPool");
    }

    auto const commandBufferAllocateInfo{ VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    }};

    if (vkAllocateCommandBuffers(*m_device, &commandBufferAllocateInfo, &m_buffer))
    {
        throw std::runtime_error("Failed to allocate VkCommandBuffer");
    }
}
