#define STB_IMAGE_IMPLEMENTATION
#include "Image.hpp"
#include "Device.hpp"
#include <volk.h>
#include <vk_mem_alloc.h>
#include <stb_image.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

vk::Image::Image()
    : m{}
{}

vk::Image::Image(Device* pDevice, std::string_view path)
{
    auto w{ i32{} }, h{ i32{} }, c{ i32{} };
    auto const imageBitmap{ stbi_load(path.data(), &w, &h, &c, STBI_rgb_alpha) };

    if (!imageBitmap)
    {
        spdlog::error("Failed to load texture: {}", path.data());
        auto whitePixel{ u32{0xffffffff} };
        *this = Image(pDevice, {1, 1}, ImageUsage::eSampled, Format::eRGBA8_unorm);
        this->write(&whitePixel, 4);
        return;
    }

    *this = Image(pDevice, {w, h}, ImageUsage::eSampled, Format::eRGBA8_unorm);
    this->write(imageBitmap, w * h * STBI_rgb_alpha);

    stbi_image_free(imageBitmap);
}

vk::Image::Image(Device* pDevice, glm::uvec2 size, ImageUsageFlags usage, Format format)
{
    m = {
        .device = pDevice,
        .usage  = usage,
        .layout = ImageLayout::eShaderRead,
        .aspect = vk::Aspect::eColor,
        .format = format,
        .size   = size
    };

    auto const imageCreateInfo{ VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = static_cast<VkFormat>(format),
        .extent = {
            .width = size.x,
            .height = size.y,
            .depth = 1u
        },
        .mipLevels = 1u,
        .arrayLayers = 1u,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    }};

    auto const allocationCreateInfo{ VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_AUTO,
        .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    }};

    auto allocationInfo{ VmaAllocationInfo{} };

    if (vmaCreateImage(*m.device, &imageCreateInfo, &allocationCreateInfo, &m.image, &m.allocation, &allocationInfo))
    {
        throw std::runtime_error("Failed to allocate image");
    }

    auto const imageViewCreateInfo{ VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = static_cast<VkFormat>(format),
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = m.aspect,
            .levelCount = 1,
            .layerCount = 1
        }
    }};

    if (vkCreateImageView(*m.device, &imageViewCreateInfo, nullptr, &m.imageView))
    {
        throw std::runtime_error("Failed to create VkImageView");
    }
}

vk::Image::~Image()
{
    if (m.device)
    {
        if (m.imageView)
        {
            vkDestroyImageView(*m.device, m.imageView, nullptr);
        }

        if (m.image && m.allocation)
        {
            vmaDestroyImage(*m.device, m.image, m.allocation);
        }
    }

    m = {};
}

vk::Image::Image(Image&& other)
    : m{ std::move(other.m) }
{
    other.m = {};
}

auto vk::Image::operator=(Image&& other) -> Image&
{
    m = std::move(other.m);
    other.m = {};

    return *this;
}

vk::Image::Image(Device* pDevice, VkImage image, Format format, glm::uvec2 size)
{
    m = {
        .device = pDevice,
        .image  = image,
        .usage  = ImageUsage::eColorAttachment,
        .layout = ImageLayout::eUndefined,
        .aspect = Aspect::eColor,
        .format = format,
        .size   = size,
    };

    auto const imageViewCreateInfo{ VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = static_cast<VkFormat>(m.format),
        .components = {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    }};

    if (vkCreateImageView(*m.device, &imageViewCreateInfo, nullptr, &m.imageView))
    {
        throw std::runtime_error("Failed to create swapchain VkImageView");
    }
}

auto vk::Image::write(void const* data, size_t dataSize) -> void
{
    auto const bufferCreateInfo{ VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = dataSize,
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

    std::memcpy(allocationInfo.pMappedData, data, dataSize);
    vmaFlushAllocation(*m.device, m.allocation, 0, dataSize);

    auto const copy{ VkBufferImageCopy{
        .imageSubresource = {
            .aspectMask = m.aspect,
            .layerCount = 1
        },
        .imageExtent = {
            .width = m.size.x,
            .height = m.size.y,
            .depth = 1
        }
    }};

    auto barrier{ VkImageMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcAccessMask = VK_QUEUE_FAMILY_IGNORED,
        .dstAccessMask = VK_QUEUE_FAMILY_IGNORED,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = m.image,
        .subresourceRange = {
            .aspectMask = m.aspect,
            .levelCount = 1,
            .layerCount = 1
        }
    }};

    auto const dependency{ VkDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    }};

    m.device->transferSubmit([&](CommandBuffer& command)
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier2(command, &dependency);

        vkCmdCopyBufferToImage(command, stagingBuffer, m.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
    
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

        vkCmdPipelineBarrier2(command, &dependency);
    });

    vmaDestroyBuffer(*m.device, stagingBuffer, stagingAllocation);
}

auto vk::Image::subwrite(void const *data, size_t dataSize, glm::ivec2 offset, glm::uvec2 size) -> void
{
}
