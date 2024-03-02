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
    : m_device{ nullptr }
    , m_queue{ nullptr }
    , m_allocator{ nullptr }
{
    this->createDevice(instance, surface, physicalDevice);
    this->createAllocator(instance, physicalDevice);
    this->createSwapchain();
}

vk::Device::~Device()
{
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
    : m_device{ other.m_device }
    , m_queue{ other.m_queue }
    , m_allocator{ other.m_allocator }
{
    other.m_device    = nullptr;
    other.m_queue     = nullptr;
    other.m_allocator = nullptr;
}

auto vk::Device::operator=(Device&& other) -> Device&
{
    m_device    = other.m_device;
    m_queue     = other.m_queue;
    m_allocator = other.m_allocator;

    other.m_device    = nullptr;
    other.m_queue     = nullptr;
    other.m_allocator = nullptr;

    return *this;
}

auto vk::Device::createDevice(Instance& instance, Surface& surface, PhysicalDevice& physicalDevice) -> void
{
    auto propertyCount{ u32{} };
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertyCount, nullptr);
    auto properties{ std::vector<VkQueueFamilyProperties>{propertyCount} };
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertyCount, properties.data());

    auto family{ u32{} };
    auto index { u32{} };

    for (auto i{ u32{} }; i < propertyCount; ++i)
    {
        auto presentSupport{ VkBool32{false} };
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

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

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &m_device))
    {
        throw std::runtime_error("Failed to create VkDevice");
    }

    volkLoadDevice(m_device);

    vkGetDeviceQueue(m_device, family, index, &m_queue);

    spdlog::info("Created device");
    spdlog::info("Graphics queue [ family: {}; index: {} ]", family, index);
}

auto vk::Device::createAllocator(Instance& instance, PhysicalDevice& physicalDevice) -> void
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
        .physicalDevice = physicalDevice,
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

}
