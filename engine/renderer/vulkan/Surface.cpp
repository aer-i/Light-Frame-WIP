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
    : m_window{ &window }
    , m_instance{ &instance }
    , m_surface{ nullptr }
{
    if (!SDL_Vulkan_CreateSurface(const_cast<SDL_Window*>(window.getHandle()), *m_instance, nullptr, &m_surface))
    {
        throw std::runtime_error("Failed to create window surface");
    }

    spdlog::info("Created window surface");
}

vk::Surface::~Surface()
{
    if (m_surface)
    {
        vkDestroySurfaceKHR(*m_instance, m_surface, nullptr);
        m_surface = nullptr;
    }

    spdlog::info("Destroyed window surface");
}

vk::Surface::Surface(Surface&& other)
    : m_window{ other.m_window }
    , m_instance{ other.m_instance }
    , m_surface{ other.m_surface }
{
    other.m_window   = nullptr;
    other.m_instance = nullptr;
    other.m_surface  = nullptr;
}

auto vk::Surface::operator=(Surface&& other) -> Surface&
{
    m_window   = other.m_window;
    m_instance = other.m_instance;
    m_surface  = other.m_surface;

    other.m_window   = nullptr;
    other.m_instance = nullptr;
    other.m_surface  = nullptr;

    return *this;
}

auto vk::Surface::getFormat(PhysicalDevice& physicalDevice) -> Format
{
    auto surfaceFormatCount{ u32{} };
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &surfaceFormatCount, nullptr);
    auto availableFormats{ std::vector<VkSurfaceFormatKHR>{surfaceFormatCount} };
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &surfaceFormatCount, availableFormats.data());

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
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &modeCount, nullptr);
    auto presentModes{ std::vector<VkPresentModeKHR>{modeCount} };
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &modeCount, presentModes.data());

    return std::ranges::any_of(
        presentModes,
        [&](auto const& presentMode){ return presentMode == static_cast<VkPresentModeKHR>(mode); }
    );
}

auto vk::Surface::getExtent(PhysicalDevice& physicalDevice) -> Extent
{
    auto capabilities{ VkSurfaceCapabilitiesKHR{} };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &capabilities);

    if (capabilities.currentExtent.width != 0xFFFFFFFF)
    {
        return Extent{
            .width  = capabilities.currentExtent.width,
            .height = capabilities.currentExtent.height
        };
    }

    return Extent{
        .width  = std::clamp(static_cast<u32>(m_window->getWidth()),  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width),
        .height = std::clamp(static_cast<u32>(m_window->getHeight()), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

auto vk::Surface::getClampedImageCount(PhysicalDevice& physicalDevice, u32 imageCount) -> u32
{
    auto capabilities{ VkSurfaceCapabilitiesKHR{} };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &capabilities);

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
