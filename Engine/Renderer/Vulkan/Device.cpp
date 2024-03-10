#define VMA_IMPLEMENTATION
#include "Device.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "Surface.hpp"
#include "Window.hpp"
#include <volk.h>
#include <vk_mem_alloc.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <stdexcept>

vk::Device::Device(Instance& instance, Surface& surface, PhysicalDevice& physicalDevice)
    : m{ 
        .surface = &surface,
        .physicalDevice = &physicalDevice,
        .surfaceFormat = surface.getFormat(physicalDevice)
    }
{
    this->createDevice(instance);
    this->createAllocator(instance);
    this->createSwapchain();
    this->createSyncObjects();
    this->createCommandBuffers();
    this->createTransferResources();
    this->createDescriptorPool();
    this->createSampler();
    
    spdlog::info("Created device");
}

vk::Device::~Device()
{
    m.transferCommandBuffer.~CommandBuffer();
    m.commandBuffers.clear();
    m.swapchainImages.clear();

    if (m.sampler)
    {
        vkDestroySampler(m.device, m.sampler, nullptr);
    }

    if (m.descriptorPool)
    {
        vkDestroyDescriptorPool(m.device, m.descriptorPool, nullptr);
    }

    if (m.transferFence)
    {
        vkDestroyFence(m.device, m.transferFence, nullptr);
    }

    for (auto i{ static_cast<u32>(m.fences.size()) }; i--; )
    {
        vkDestroyFence(m.device, m.fences[i], nullptr);
        vkDestroySemaphore(m.device, m.presentSemaphores[i], nullptr);
        vkDestroySemaphore(m.device, m.renderSemaphores[i], nullptr);
    }

    if (m.swapchain)
    {
        vkDestroySwapchainKHR(m.device, m.swapchain, nullptr);
    }

    if (m.allocator)
    {
        vmaDestroyAllocator(m.allocator);
    }

    if (m.device)
    {
        vkDestroyDevice(m.device, nullptr);
    }

    m = {};

    spdlog::info("Destroyed device");
}

vk::Device::Device(Device&& other)
    : m{ std::move(other.m) }
{
    other.m = {};
}

auto vk::Device::operator=(Device&& other) -> Device&
{
    m = std::move(other.m);
    other.m = {};

    return *this;
}

auto vk::Device::waitIdle() -> void
{
    vkDeviceWaitIdle(m.device);
}

auto vk::Device::waitForFences() -> void
{
    m.frameIndex = (m.frameIndex + 1) % m.imageCount;

    vkWaitForFences(m.device, 1, &m.fences[m.frameIndex], 0u, ~0ull);
    vkResetFences(m.device, 1, &m.fences[m.frameIndex]);
}

auto vk::Device::checkSwapchainState(Window &window) -> bool
{
    static auto previousSize{ window.getSize() };

    if (previousSize != window.getSize())
    {
        while (window.getWidth() < 1 || window.getHeight() < 1)
        {
            window.update();

            if (!window.available())
            {
                return false;
            }
        }

        vkDeviceWaitIdle(m.device);
        this->createSwapchain();
        previousSize = window.getSize();
        
        return true;
    }

    return false;
}

auto vk::Device::acquireImage() -> void
{
    switch (vkAcquireNextImageKHR(m.device, m.swapchain, ~0ull, m.renderSemaphores[m.frameIndex], nullptr, &m.imageIndex))
    {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        vkDeviceWaitIdle(m.device);
        this->createSwapchain();
        break;
    default:
        throw std::runtime_error("Failed to acquire next swapchain images");
        break;
    }
}

auto vk::Device::submitCommands(ArrayProxy<CommandBuffer::Handle> const& commands) -> void
{
    auto const waitStage{ VkPipelineStageFlags{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT} };

    auto const submitInfo{ VkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m.renderSemaphores[m.frameIndex],
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = commands.size(),
        .pCommandBuffers = commands.data(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m.presentSemaphores[m.frameIndex]
    }};

    if (vkQueueSubmit(m.queue, 1, &submitInfo, m.fences[m.frameIndex]))
    {
        throw std::runtime_error("Failed to submit command buffers");
    }
}

auto vk::Device::present() -> void
{
    auto const presentInfo{ VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m.presentSemaphores[m.frameIndex],
        .swapchainCount = 1,
        .pSwapchains = &m.swapchain,
        .pImageIndices = &m.imageIndex
    }};

    switch (vkQueuePresentKHR(m.queue, &presentInfo))
    {
    case VK_SUCCESS:
        break;
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
        vkDeviceWaitIdle(m.device);
        this->createSwapchain();
        break;
    default:
        throw std::runtime_error("Failed to present frame");
        break;
    }
}

auto vk::Device::transferSubmit(std::function<void(CommandBuffer&)>&& function) -> void
{
    vkResetFences(m.device, 1, &m.transferFence);

    m.transferCommandBuffer.begin();
    {
        function(m.transferCommandBuffer);
    }
    m.transferCommandBuffer.end();

    auto const command{ VkCommandBuffer{m.transferCommandBuffer} };
    auto const submitInfo{ VkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command
    }};

    if (vkQueueSubmit(m.queue, 1, &submitInfo, m.transferFence))
    {
        throw std::runtime_error("Failed to submit command buffers");
    }

    vkWaitForFences(m.device, 1, &m.transferFence, 0, ~0ull);
}

auto vk::Device::createDevice(Instance& instance) -> void
{
    auto propertyCount{ u32{} };
    vkGetPhysicalDeviceQueueFamilyProperties(*m.physicalDevice, &propertyCount, nullptr);
    auto properties{ std::vector<VkQueueFamilyProperties>{propertyCount} };
    vkGetPhysicalDeviceQueueFamilyProperties(*m.physicalDevice, &propertyCount, properties.data());

    auto family{ u32{} };
    auto index { u32{} };

    for (auto i{ u32{} }; i < propertyCount; ++i)
    {
        auto presentSupport{ VkBool32{false} };
        vkGetPhysicalDeviceSurfaceSupportKHR(*m.physicalDevice, i, *m.surface, &presentSupport);

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
        .descriptorIndexing = true,
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

    if (vkCreateDevice(*m.physicalDevice, &deviceCreateInfo, nullptr, &m.device))
    {
        throw std::runtime_error("Failed to create VkDevice");
    }

    volkLoadDevice(m.device);

    vkGetDeviceQueue(m.device, family, index, &m.queue);

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
        .physicalDevice = *m.physicalDevice,
        .device = m.device,
        .pVulkanFunctions = &functions,
        .instance = instance,
        .vulkanApiVersion = instance.apiVersion(),
    }};

    if (vmaCreateAllocator(&allocatorCreateInfo, &m.allocator))
    {
        throw std::runtime_error("Failed to create vulkan memory allocator");
    }
}

auto vk::Device::createSwapchain() -> void
{
    m.swapchainImages.clear();
    m.surfaceFormat = m.surface->getFormat(*m.physicalDevice);

    auto extent{ m.surface->getExtent(*m.physicalDevice) };
    auto minImageCount{ m.surface->getClampedImageCount(*m.physicalDevice, 3u) };
    auto presentMode{ PresentMode::eFifo };

    m.swapchainExtent = {
        extent.width, extent.height
    };

    if (m.surface->presentModeSupport(*m.physicalDevice, PresentMode::eImmediate))
    {
        presentMode = PresentMode::eImmediate;
    }

    if (m.surface->presentModeSupport(*m.physicalDevice, PresentMode::eMailbox))
    {
        presentMode = PresentMode::eMailbox;
    }

    m.oldSwapchain = m.swapchain;

    auto const swapchainCreateInfo{ VkSwapchainCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = *m.surface,
        .minImageCount = minImageCount,
        .imageFormat = static_cast<VkFormat>(m.surfaceFormat),
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = {.width = extent.width, .height = extent.height},
        .imageArrayLayers = 1u,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = static_cast<VkPresentModeKHR>(presentMode),
        .clipped = 1u,
        .oldSwapchain = m.oldSwapchain
    }};

    if (vkCreateSwapchainKHR(m.device, &swapchainCreateInfo, nullptr, &m.swapchain))
    {
        throw std::runtime_error("Failed to create VkSwapchain");
    }

    if (m.oldSwapchain)
    {
        vkDestroySwapchainKHR(m.device, m.oldSwapchain, nullptr);
        m.oldSwapchain = nullptr;
    }

    auto imageCount{ u32{} };
    vkGetSwapchainImagesKHR(m.device, m.swapchain, &imageCount, nullptr);
    auto images{ std::vector<VkImage>{imageCount} };
    vkGetSwapchainImagesKHR(m.device, m.swapchain, &imageCount, images.data());

    m.swapchainImages = std::vector<Image>{ imageCount };

    for (auto i{ imageCount }; i--; )
    {
        m.swapchainImages[i] = Image(this, images[i], m.surfaceFormat, {extent.width, extent.height});
    }
}

auto vk::Device::createCommandBuffers() -> void
{
    m.imageCount = static_cast<u32>(m.swapchainImages.size());
    m.commandBuffers = std::vector<CommandBuffer>{ m.swapchainImages.size() };

    vkWaitForFences(m.device, 1, &m.fences[m.frameIndex], 0u, ~0ull);
    vkResetFences(m.device, 1, &m.fences[m.frameIndex]);

    switch (vkAcquireNextImageKHR(m.device, m.swapchain, ~0ull, m.renderSemaphores[m.frameIndex], nullptr, &m.imageIndex))
    {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        vkDeviceWaitIdle(m.device);
        this->createSwapchain();
        break;
    default:
        throw std::runtime_error("Failed to acquire next swapchain images");
        break;
    }

    for (auto& commandBuffer : m.commandBuffers)
    {
        commandBuffer.allocate(this);
        commandBuffer.begin();
        commandBuffer.beginPresent();
        commandBuffer.endPresent();
        commandBuffer.end();
    }
}

auto vk::Device::createSyncObjects() -> void
{
    m.presentSemaphores = std::vector<VkSemaphore>{ m.swapchainImages.size() };
    m.renderSemaphores  = std::vector<VkSemaphore>{ m.swapchainImages.size() };
    m.fences            = std::vector<VkFence>{ m.swapchainImages.size() };

    for (auto i{ m.presentSemaphores.size() }; i--; )
    {
        auto const semaphoreCreateInfo{ VkSemaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        }};

        auto const fenceCreateInfo{ VkFenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        }};

        if (vkCreateSemaphore(m.device, &semaphoreCreateInfo, nullptr, &m.presentSemaphores[i]) ||
            vkCreateSemaphore(m.device, &semaphoreCreateInfo, nullptr, &m.renderSemaphores[i]))
        {
            throw std::runtime_error("Failed to create VkSemaphore");
        }

        if (vkCreateFence(m.device, &fenceCreateInfo, nullptr, &m.fences[i]))
        {
            throw std::runtime_error("Failed to create VkFence");
        }
    }
}

auto vk::Device::createTransferResources() -> void
{
    m.transferCommandBuffer.allocateForTransfers(this);

    auto const fenceCreateInfo{ VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    }};

    if (vkCreateFence(m.device, &fenceCreateInfo, nullptr, &m.transferFence))
    {
        throw std::runtime_error("Failed to create VkFence");
    }
}

auto vk::Device::createDescriptorPool() -> void
{
    auto const poolSizes{ std::array{
        VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .descriptorCount = 1024 * 64 },
        VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .descriptorCount = 1024 * 64 },
        VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1024 * 64 }
    }};

    auto const descriptorPoolCreateInfo{ VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets = 1024,
        .poolSizeCount = static_cast<u32>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    }};

    if (vkCreateDescriptorPool(m.device, &descriptorPoolCreateInfo, nullptr, &m.descriptorPool))
    {
        throw std::runtime_error("Failed to create VkDescriptorPool");
    }
}

auto vk::Device::createSampler() -> void
{
    auto const samplerCreateInfo{ VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV= VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    }};

    if (vkCreateSampler(m.device, &samplerCreateInfo, nullptr, &m.sampler))
    {
        throw std::runtime_error("Failed to create VkSampler");
    }
}
