#define VMA_IMPLEMENTATION
#include "Device.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "Surface.hpp"
#include "Types.hpp"
#include <volk.h>
#include <vk_mem_alloc.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <stdexcept>

vk::Device::Device(Instance& instance, Surface& surface, PhysicalDevice& physicalDevice)
    : m_surface{ &surface }
    , m_physicalDevice{ &physicalDevice }
    , m_device{ nullptr }
    , m_queue{ nullptr }
    , m_swapchain{ nullptr }
    , m_allocator{ nullptr }
    , m_frameIndex{ u32{} }
{
    this->createDevice(instance);
    this->createAllocator(instance);
    this->createSwapchain();
    this->createCommandBuffers();
    this->createSyncObjects();
    
    spdlog::info("Created device");
}

vk::Device::~Device()
{
    vkDeviceWaitIdle(m_device);

    m_commandBuffers.clear();
    m_swapchainImages.clear();

    for (auto i{ static_cast<u32>(m_fences.size()) }; i--; )
    {
        vkDestroyFence(m_device, m_fences[i], nullptr);
        vkDestroySemaphore(m_device, m_presentSemaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_renderSemaphores[i], nullptr);
    }

    if (m_swapchain)
    {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = nullptr;
    }

    if (m_allocator)
    {
        vmaDestroyAllocator(m_allocator);
        m_allocator = nullptr;
    }

    if (m_device)
    {
        vkDestroyDevice(m_device, nullptr);
        m_device = nullptr;
    }

    spdlog::info("Destroyed device");
}

vk::Device::Device(Device&& other)
    : m_surface{ other.m_surface }
    , m_physicalDevice{ other.m_physicalDevice }
    , m_device{ other.m_device }
    , m_queue{ other.m_queue }
    , m_swapchain{ other.m_swapchain }
    , m_allocator{ other.m_allocator }
{
    other.m_surface        = nullptr;
    other.m_physicalDevice = nullptr;
    other.m_device         = nullptr;
    other.m_queue          = nullptr;
    other.m_swapchain      = nullptr;
    other.m_allocator      = nullptr;
}

auto vk::Device::operator=(Device&& other) -> Device&
{
    m_surface         = other.m_surface;
    m_physicalDevice  = other.m_physicalDevice;
    m_device          = other.m_device;
    m_queue           = other.m_queue;
    m_swapchain       = other.m_swapchain;
    m_allocator       = other.m_allocator;

    other.m_surface        = nullptr;
    other.m_physicalDevice = nullptr;
    other.m_device         = nullptr;
    other.m_queue          = nullptr;
    other.m_swapchain      = nullptr;
    other.m_allocator      = nullptr;

    return *this;
}

auto vk::Device::acquireImage() -> void
{
    vkWaitForFences(m_device, 1, &m_fences[m_frameIndex], 0u, ~0ull);
    vkResetFences(m_device, 1, &m_fences[m_frameIndex]);

    vkQueueWaitIdle(m_queue);

    vkAcquireNextImageKHR(m_device, m_swapchain, ~0ull, m_renderSemaphores[m_frameIndex], nullptr, &m_imageIndex);
}

auto vk::Device::submitCommands(ArrayProxy<CommandBuffer::Handle> const& commands) -> void
{
    auto const waitStage{ VkPipelineStageFlags{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT} };

    auto const submitInfo{ VkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_renderSemaphores[m_frameIndex],
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = commands.size(),
        .pCommandBuffers = commands.data(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_presentSemaphores[m_frameIndex]
    }};

    if (vkQueueSubmit(m_queue, 1, &submitInfo, m_fences[m_frameIndex]))
    {
        throw std::runtime_error("Failed to submit command buffers");
    }
}

auto vk::Device::present() -> void
{
    auto const presentInfo{ VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_presentSemaphores[m_frameIndex],
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain,
        .pImageIndices = &m_imageIndex
    }};

    if (vkQueuePresentKHR(m_queue, &presentInfo))
    {
        throw std::runtime_error("Failed to present frame");
    }

    m_frameIndex = (m_frameIndex + 1) % m_commandBuffers.size();
}

auto vk::Device::createDevice(Instance& instance) -> void
{
    auto propertyCount{ u32{} };
    vkGetPhysicalDeviceQueueFamilyProperties(*m_physicalDevice, &propertyCount, nullptr);
    auto properties{ std::vector<VkQueueFamilyProperties>{propertyCount} };
    vkGetPhysicalDeviceQueueFamilyProperties(*m_physicalDevice, &propertyCount, properties.data());

    auto family{ u32{} };
    auto index { u32{} };

    for (auto i{ u32{} }; i < propertyCount; ++i)
    {
        auto presentSupport{ VkBool32{false} };
        vkGetPhysicalDeviceSurfaceSupportKHR(*m_physicalDevice, i, *m_surface, &presentSupport);

        if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
            properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT &&
            presentSupport)
        {
            family = i;
            index = 0;
            break;
        }
    }

    auto const queuePriority{ f32{1.f} };

    auto const queueCreateInfo{ VkDeviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = family,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    }};

    auto vulkan11Features{ VkPhysicalDeviceVulkan11Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .storageBuffer16BitAccess = true,
        .shaderDrawParameters = true
    }};

    auto vulkan12Features{ VkPhysicalDeviceVulkan12Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &vulkan11Features,
        .drawIndirectCount = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        .descriptorBindingPartiallyBound = true,
        .runtimeDescriptorArray = true
    }};

    auto vulkan13Features{ VkPhysicalDeviceVulkan13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &vulkan12Features,
        .synchronization2 = true,
        .dynamicRendering = true
    }};

    auto enabledFeatures{ VkPhysicalDeviceFeatures2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &vulkan13Features,
        .features = {
            .multiDrawIndirect = true,
            .fillModeNonSolid = true
        }
    }};

    auto const swapchainExtension{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    auto const deviceCreateInfo{ VkDeviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabledFeatures,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = &swapchainExtension
    }};

    if (vkCreateDevice(*m_physicalDevice, &deviceCreateInfo, nullptr, &m_device))
    {
        throw std::runtime_error("Failed to create VkDevice");
    }

    volkLoadDevice(m_device);

    vkGetDeviceQueue(m_device, family, index, &m_queue);

    spdlog::info("Graphics queue [ family: {}; index: {} ]", family, index);
}

auto vk::Device::createAllocator(Instance& instance) -> void
{
    auto const functions{ VmaVulkanFunctions{
        .vkGetInstanceProcAddr                   = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr                     = vkGetDeviceProcAddr,
        .vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory                        = vkAllocateMemory,
        .vkFreeMemory                            = vkFreeMemory,
        .vkMapMemory                             = vkMapMemory,
        .vkUnmapMemory                           = vkUnmapMemory,
        .vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory                      = vkBindBufferMemory,
        .vkBindImageMemory                       = vkBindImageMemory,
        .vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements,
        .vkCreateBuffer                          = vkCreateBuffer,
        .vkDestroyBuffer                         = vkDestroyBuffer,
        .vkCreateImage                           = vkCreateImage,
        .vkDestroyImage                          = vkDestroyImage,
        .vkCmdCopyBuffer                         = vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2,
        .vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2,
        .vkBindBufferMemory2KHR                  = vkBindBufferMemory2,
        .vkBindImageMemory2KHR                   = vkBindImageMemory2,
        .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2,
        .vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements,
        .vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements
    }};

    auto const allocatorCreateInfo{ VmaAllocatorCreateInfo{
        .physicalDevice = *m_physicalDevice,
        .device = m_device,
        .pVulkanFunctions = &functions,
        .instance = instance,
        .vulkanApiVersion = instance.apiVersion(),
    }};

    if (vmaCreateAllocator(&allocatorCreateInfo, &m_allocator))
    {
        throw std::runtime_error("Failed to create vulkan memory allocator");
    }
}

auto vk::Device::createSwapchain() -> void
{
    auto format{ m_surface->getFormat(*m_physicalDevice) };
    auto extent{ m_surface->getExtent(*m_physicalDevice) };
    auto minImageCount{ m_surface->getClampedImageCount(*m_physicalDevice, 3u) };
    auto presentMode{ PresentMode::eFifo };

    if (m_surface->presentModeSupport(*m_physicalDevice, PresentMode::eImmediate))
    {
        presentMode = PresentMode::eImmediate;
    }

    if (m_surface->presentModeSupport(*m_physicalDevice, PresentMode::eMailbox))
    {
        presentMode = PresentMode::eMailbox;
    }

    auto const swapchainCreateInfo{ VkSwapchainCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = *m_surface,
        .minImageCount = minImageCount,
        .imageFormat = static_cast<VkFormat>(format),
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = {.width = extent.width, .height = extent.height},
        .imageArrayLayers = 1u,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = static_cast<VkPresentModeKHR>(presentMode),
        .clipped = 1u
    }};

    if (vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain))
    {
        throw std::runtime_error("Failed to create VkSwapchain");
    }

    auto imageCount{ u32{} };
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    auto images{ std::vector<VkImage>{imageCount} };
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, images.data());

    m_swapchainImages = std::vector<Image>{ imageCount };

    for (auto i{ imageCount }; i--; )
    {
        m_swapchainImages[i].loadFromSwapchain(this, images[i], format, {extent.width, extent.height});
    }
}

auto vk::Device::createCommandBuffers() -> void
{
    m_commandBuffers = std::vector<CommandBuffer>{ m_swapchainImages.size() };

    for (auto& commandBuffer : m_commandBuffers)
    {
        commandBuffer.allocate(this);
    }
}

auto vk::Device::createSyncObjects() -> void
{
    m_presentSemaphores = std::vector<VkSemaphore>{ m_swapchainImages.size() };
    m_renderSemaphores  = std::vector<VkSemaphore>{ m_swapchainImages.size() };
    m_fences            = std::vector<VkFence>{ m_swapchainImages.size() };

    for (auto i{ u32{} }; i < m_presentSemaphores.size(); ++i)
    {
        auto const semaphoreCreateInfo{ VkSemaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        }};

        auto const fenceCreateInfo{ VkFenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        }};

        if (vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_presentSemaphores[i]) ||
            vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_renderSemaphores[i]))
        {
            throw std::runtime_error("Failed to create VkSemaphore");
        }

        if (vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_fences[i]))
        {
            throw std::runtime_error("Failed to create VkFence");
        }
    }
}
