#include "PhysicalDevice.hpp"
#include "Instance.hpp"
#include "Types.hpp"
#include <volk.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <stdexcept>

vk::PhysicalDevice::PhysicalDevice(Instance& instance)
    : m_physicalDevice{ nullptr }
{
    auto vulkan_1_3Features{ VkPhysicalDeviceVulkan13Features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES} };
    auto meshShaderFeatures{ VkPhysicalDeviceMeshShaderFeaturesEXT{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT} };
    auto properties        { VkPhysicalDeviceProperties2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2} };
    auto features          { VkPhysicalDeviceFeatures2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2} };

    features.pNext = &meshShaderFeatures;
    meshShaderFeatures.pNext = &vulkan_1_3Features;

    auto deviceCount{ u32{} };
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    auto physicalDevices{ std::vector<VkPhysicalDevice>{deviceCount} };
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    if (physicalDevices.empty())
    {
        throw std::runtime_error("No available graphics cards");
    }

    for (auto const currentPhysicalDevice : physicalDevices)
    {
        vkGetPhysicalDeviceProperties2(currentPhysicalDevice, &properties);
        vkGetPhysicalDeviceFeatures2(currentPhysicalDevice, &features);

        if (properties.properties.apiVersion < VK_API_VERSION_1_3) continue;
        if (!features.features.geometryShader)                     continue;
        if (!features.features.tessellationShader)                 continue;
        if (!vulkan_1_3Features.synchronization2)                  continue;
        if (!vulkan_1_3Features.dynamicRendering)                  continue;

        m_physicalDevice = currentPhysicalDevice;

        if (meshShaderFeatures.meshShader) break;
    }

    if (!m_physicalDevice)
    {
        throw std::runtime_error("Could not find any suitable graphics card. Make sure you have installed the latest drivers");
    }

    spdlog::info("Selected gprahics card [ {} ]", properties.properties.deviceName);
    spdlog::info(
        "Graphics card driver version [ {}.{}.{} ]",
        VK_VERSION_MAJOR(properties.properties.driverVersion),
        VK_VERSION_MINOR(properties.properties.driverVersion),
        VK_VERSION_PATCH(properties.properties.driverVersion)
    );
    spdlog::info(
        "Graphics card API version [ {}.{}.{} ]",
        VK_VERSION_MAJOR(properties.properties.apiVersion),
        VK_VERSION_MINOR(properties.properties.apiVersion),
        VK_VERSION_PATCH(properties.properties.apiVersion)
    );
}

vk::PhysicalDevice::PhysicalDevice(PhysicalDevice&& other)
    : m_physicalDevice{ other.m_physicalDevice }
{
    other.m_physicalDevice = nullptr;
}

auto vk::PhysicalDevice::operator=(PhysicalDevice&& other) -> PhysicalDevice&
{
    m_physicalDevice = other.m_physicalDevice;

    other.m_physicalDevice = nullptr;

    return *this;
}
