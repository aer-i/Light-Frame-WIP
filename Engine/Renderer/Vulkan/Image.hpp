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
        Image(Device* pDevice, std::string_view path);
        Image(Device* pDevice, glm::uvec2 size, ImageUsageFlags usage, Format format);
        ~Image();
        Image(Image const&) = delete;
        Image(Image&& other);
        auto operator=(Image const&) -> Image& = delete;
        auto operator=(Image&& other) -> Image&;

    public:
        auto write(void const* data, size_t dataSize) -> void;
        auto subwrite(void const* data, size_t dataSize, glm::ivec2 offset, glm::uvec2 size) -> void;

    private:
        friend class Device;
        Image(Device* pDevice, VkImage image, Format format, glm::uvec2 size);

    public:
        inline operator VkImage() const noexcept
        {
            return m.image;
        }

        inline operator VkImageView() const noexcept
        {
            return m.imageView;
        }

        inline auto getSize() const noexcept -> glm::uvec2
        {
            return m.size;
        }

        inline auto getWidth() const noexcept -> u32
        {
            return m.size.x;
        }

        inline auto getHeight() const noexcept -> u32
        {
            return m.size.y;
        }

        inline auto getLayout() const noexcept -> ImageLayout
        {
            return m.layout;
        }

        inline auto getAspect() const noexcept -> AspectFlags
        {
            return m.aspect;
        }

        inline auto getUsage() const noexcept -> ImageUsageFlags
        {
            return m.usage;
        }

        inline auto getFormat() const noexcept -> Format
        {
            return m.format;
        }

        inline auto setLayout(ImageLayout layout) noexcept -> void
        {
            m.layout = layout;
        }

    private:
        struct M
        {
            Device*         device;
            VkImage         image;
            VkImageView     imageView;
            VmaAllocation   allocation;
            ImageUsageFlags usage;
            ImageLayout     layout;
            AspectFlags     aspect;
            Format          format;
            glm::uvec2      size;
        } m;
    };
}