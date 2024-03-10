#include "Surface.hpp"
#include "Window.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include <volk.h>
#include <SDL3/SDL_vulkan.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <algorithm>

vk::Surface::Surface(Window& window, Instance& instance)
    : m{
        .window = &window,
        .instance = &instance
    }
{
    if (!SDL_Vulkan_CreateSurface(const_cast<SDL_Window*>(window.getHandle()), *m.instance, nullptr, &m.surface))
    {
        throw std::runtime_error("Failed to create window surface");
    }

    spdlog::info("Created window surface");
}

vk::Surface::~Surface()
{
    if (m.surface)
    {
        vkDestroySurfaceKHR(*m.instance, m.surface, nullptr);
    }

    m = {};

    spdlog::info("Destroyed window surface");
}

vk::Surface::Surface(Surface&& other)
    : m{ std::move(other.m) }
{
    other.m = {};
}

auto vk::Surface::operator=(Surface&& other) -> Surface&
{
    m = std::move(other.m);
    other.m = {};

    return *this;
}

auto vk::Surface::getFormat(PhysicalDevice& physicalDevice) -> Format
{
    auto surfaceFormatCount{ u32{} };
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m.surface, &surfaceFormatCount, nullptr);
    auto availableFormats{ std::vector<VkSurfaceFormatKHR>{surfaceFormatCount} };
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m.surface, &surfaceFormatCount, availableFormats.data());

    if (availableFormats.empty())
    {
        throw std::runtime_error("No available surface formats");
    }

    if (availableFormats.size() == 1 && availableFormats.front().format == VK_FORMAT_UNDEFINED)
    {
        return Format::eRGBA8_unorm;
    }

    for (auto const& currentFormat : availableFormats)
    {
        if (currentFormat.format == VK_FORMAT_R8G8B8A8_UNORM && currentFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return Format::eRGBA8_unorm;
        }
    }

    for (auto const& currentFormat : availableFormats)
    {
        if (currentFormat.format == VK_FORMAT_B8G8R8A8_UNORM && currentFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return Format::eBGRA8_unorm;
        }
    }

    return static_cast<Format>(availableFormats.front().format);
}

auto vk::Surface::presentModeSupport(PhysicalDevice& physicalDevice, PresentMode mode) -> bool
{
    auto modeCount{ u32{} };
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m.surface, &modeCount, nullptr);
    auto presentModes{ std::vector<VkPresentModeKHR>{modeCount} };
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m.surface, &modeCount, presentModes.data());

    return std::ranges::any_of(
        presentModes,
        [&](auto const& presentMode){ return presentMode == static_cast<VkPresentModeKHR>(mode); }
    );
}

auto vk::Surface::getExtent(PhysicalDevice& physicalDevice) -> Extent
{
    auto capabilities{ VkSurfaceCapabilitiesKHR{} };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m.surface, &capabilities);

    if (capabilities.currentExtent.width != 0xFFFFFFFF)
    {
        return Extent{
            .width  = capabilities.currentExtent.width,
            .height = capabilities.currentExtent.height
        };
    }

    return Extent{
        .width  = std::clamp(static_cast<u32>(m.window->getWidth()),  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width),
        .height = std::clamp(static_cast<u32>(m.window->getHeight()), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

auto vk::Surface::getClampedImageCount(PhysicalDevice& physicalDevice, u32 imageCount) -> u32
{
    auto capabilities{ VkSurfaceCapabilitiesKHR{} };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m.surface, &capabilities);

    if (imageCount < capabilities.minImageCount)
    {
        imageCount = capabilities.minImageCount;
    }

    if (capabilities.maxImageCount && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    return imageCount;
}
