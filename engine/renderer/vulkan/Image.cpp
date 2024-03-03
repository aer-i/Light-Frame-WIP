#include "Image.hpp"
#include "Device.hpp"
#include <volk.h>
#include <vk_mem_alloc.h>
#include <stdexcept>

vk::Image::Image()
    : m_device{ nullptr }
    , m_image{ nullptr }
    , m_imageView{ nullptr }
    , m_allocation{ nullptr }
{}

vk::Image::~Image()
{
    if (m_device)
    {
        if (m_imageView)
        {
            vkDestroyImageView(*m_device, m_imageView, nullptr);

            m_imageView = nullptr;
        }

        if (m_image && m_allocation)
        {
            vmaDestroyImage(*m_device, m_image, m_allocation);
            
            m_image      = nullptr;
            m_allocation = nullptr;
        }
    }
}

vk::Image::Image(Image&& other)
    : m_device{ other.m_device }
    , m_image{ other.m_image }
    , m_imageView{ other.m_imageView }
    , m_allocation{ other.m_allocation }
    , m_usage{ other.m_usage }
    , m_layout{ other.m_layout }
    , m_aspect{ other.m_aspect }
    , m_format{ other.m_format }
    , m_size{ other.m_size }
{
    other.m_device     = nullptr;
    other.m_image      = nullptr;
    other.m_imageView  = nullptr;
    other.m_allocation = nullptr;
}

auto vk::Image::operator=(Image&& other) -> Image&
{
    m_device     = other.m_device;
    m_image      = other.m_image;
    m_imageView  = other.m_imageView;
    m_allocation = other.m_allocation;
    m_usage      = other.m_usage;
    m_layout     = other.m_layout;
    m_aspect     = other.m_aspect;
    m_format     = other.m_format;
    m_size       = other.m_size;

    other.m_device     = nullptr;
    other.m_image      = nullptr;
    other.m_imageView  = nullptr;
    other.m_allocation = nullptr;

    return *this;
}

auto vk::Image::loadFromFile(std::string_view path) -> void
{
}

auto vk::Image::allocate(Device* pDevice, glm::uvec2 size, ImageUsageFlags usage, Format format) -> void
{
    m_device = pDevice;
    m_size   = size;
    m_usage  = usage;
    m_format = format;
    m_layout = ImageLayout::eShaderRead;
}

auto vk::Image::write(void const *data, size_t dataSize) -> void
{

}

auto vk::Image::subwrite(void const *data, size_t dataSize, glm::ivec2 offset, glm::uvec2 size) -> void
{
}

auto vk::Image::loadFromSwapchain(Device* pDevice, VkImage image, Format format, glm::uvec2 size) -> void
{
    m_device = pDevice;
    m_image  = image;
    m_format = format;
    m_size   = size;
    m_layout = ImageLayout::eUndefined;
    m_aspect = Aspect::eColor;
    m_usage  = ImageUsage::eColorAttachment;

    auto const imageViewCreateInfo{ VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = static_cast<VkFormat>(m_format),
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

    if (vkCreateImageView(*m_device, &imageViewCreateInfo, nullptr, &m_imageView))
    {
        throw std::runtime_error("Failed to create swapchain VkImageView");
    }
}
