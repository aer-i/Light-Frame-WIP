#pragma once
#include "Types.hpp"
#include "VulkanEnums.hpp"
#include <glm/glm.hpp>
#include <string_view>

struct VkImage_T;
struct VkImageView_T;
struct VmaAllocation_T;

using VkImage       = VkImage_T*;
using VkImageView   = VkImageView_T*;
using VmaAllocation = VmaAllocation_T*;

namespace vk
{
    class Device;

    class Image
    {
    public:
        Image();
        ~Image();
        Image(Image const&) = delete;
        Image(Image&& other);
        auto operator=(Image const&) -> Image& = delete;
        auto operator=(Image&& other) -> Image&;

    public:
        auto loadFromFile(std::string_view path) -> void;
        auto allocate(Device* pDevice, glm::uvec2 size, ImageUsageFlags usage, Format format) -> void;
        auto write(void const* data, size_t dataSize) -> void;
        auto subwrite(void const* data, size_t dataSize, glm::ivec2 offset, glm::uvec2 size) -> void;

    private:
        friend class Device;
        auto loadFromSwapchain(Device* pDevice, VkImage image, Format format, glm::uvec2 size) -> void;

    public:
        inline operator VkImage() const noexcept
        {
            return m_image;
        }

        inline operator VkImageView() const noexcept
        {
            return m_imageView;
        }

        inline auto getSize() const noexcept -> glm::uvec2
        {
            return m_size;
        }

        inline auto getWidth() const noexcept -> u32
        {
            return m_size.x;
        }

        inline auto getHeight() const noexcept -> u32
        {
            return m_size.y;
        }

        inline auto getLayout() const noexcept -> ImageLayout
        {
            return m_layout;
        }

        inline auto getAspect() const noexcept -> AspectFlags
        {
            return m_aspect;
        }

    private:
        Device*         m_device;
        VkImage         m_image;
        VkImageView     m_imageView;
        VmaAllocation   m_allocation;
        ImageUsageFlags m_usage;
        ImageLayout     m_layout;
        AspectFlags     m_aspect;
        Format          m_format;
        glm::uvec2      m_size;
    };
}