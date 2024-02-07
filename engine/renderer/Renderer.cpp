#define VMA_IMPLEMENTATION
#include "Renderer.hpp"
#include <aixlog.hpp>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <array>
#include <vector>
#include <memory>
#include <string>
#include <cstring>
#include "Types.hpp"
#include "Window.hpp"

static VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, u64, size_t, i32, char const*, char const*, void*);
static auto vkErrorString(VkResult result) -> char const*;
static auto vkCheck(VkResult result) -> void;

namespace vk
{
    struct QueueHandle
    {
        VkQueue queue;
        u32 index, family;
    };

    class Context
    {
    public:
        auto createInstance()       -> void;
        auto createPhysicalDevice() -> void;
        auto createDevice()         -> void;
        auto createAllocator()      -> void;
        auto teardown()             -> void;

    public:
        VmaAllocator             allocator;
        VkInstance               instance;
        VkDebugReportCallbackEXT debugCallback;
        VkSurfaceKHR             surface;
        VkPhysicalDevice         physicalDevice;
        VkDevice                 device;
        VkSwapchainKHR           swapchain;
        VkExtent2D               extent;
        QueueHandle              graphicsQueue;
    };

    Context static g_context;

    auto init() -> void
    {
        std::memset(&g_context, 0, sizeof(Context));
        g_context.createInstance();
        g_context.createPhysicalDevice();
        g_context.createDevice();
        g_context.createAllocator();
    }

    auto teardown() -> void
    {
        g_context.teardown();
    }
    
    auto Context::createInstance() -> void
    {
        vkCheck(volkInitialize());

        u32 extensionCount;
        auto extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);

        std::vector<char const*> extensions(extensionNames, extensionNames + extensionCount);
        std::vector<char const*> layers;

        #ifndef NDEBUG
        constexpr bool enableValidation = true;
        #else
        constexpr bool enableValidation = false;
        #endif

        if constexpr (enableValidation)
        {
            layers.emplace_back("VK_LAYER_KHRONOS_validation");
            extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        VkValidationFeatureEnableEXT enabledValidationFeatures[] =
        {
            VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
        };

        VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
        validationFeatures.enabledValidationFeatureCount = 2;
        validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

        VkApplicationInfo applicationInfo;
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pNext = nullptr;
        applicationInfo.apiVersion = VK_API_VERSION_1_3;
        applicationInfo.pEngineName = "Light Frame";
        applicationInfo.pApplicationName = "Light Frame Application";
        applicationInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        applicationInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);

        VkInstanceCreateInfo instanceCreateInfo;
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pNext = nullptr;
        instanceCreateInfo.flags = 0;
        instanceCreateInfo.pApplicationInfo = &applicationInfo;
        instanceCreateInfo.enabledLayerCount = static_cast<u32>(layers.size());
        instanceCreateInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
        instanceCreateInfo.ppEnabledLayerNames = layers.data();

        if constexpr (enableValidation)
        {
            instanceCreateInfo.pNext = &validationFeatures;
        }

        vkCheck(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
        volkLoadInstance(instance);

        if constexpr (enableValidation)
        {
            VkDebugReportCallbackCreateInfoEXT reportCallbackInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
            reportCallbackInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
            reportCallbackInfo.pfnCallback = debugReportCallback;

            vkCheck(vkCreateDebugReportCallbackEXT(instance, &reportCallbackInfo, nullptr, &debugCallback));
            LOG(INFO, "Vulkan Context") << "Enabled validation layers\n";
        }

        vkCheck(glfwCreateWindowSurface(instance, (GLFWwindow*)Window::GetHandle(), nullptr, &surface));

        LOG(INFO, "Vulkan Context") << "Created instance\n";
    }

    auto Context::createPhysicalDevice() -> void
    {
        u32 deviceCount;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

        if (physicalDevices.empty())
        {
            throw std::runtime_error("No available graphics cards");
        }

        for (auto const gpu : physicalDevices)
        {
            VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
            VkPhysicalDeviceProperties2 properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            VkPhysicalDeviceFeatures2 features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

            features.pNext = &meshShaderFeatures;

            vkGetPhysicalDeviceProperties2(gpu, &properties);
            vkGetPhysicalDeviceFeatures2(gpu, &features);

            if (properties.properties.apiVersion < VK_API_VERSION_1_3) continue;
            if (features.features.geometryShader == false)             continue;

            physicalDevice = gpu;

            if (meshShaderFeatures.meshShader) break;
        }

        LOG(INFO, "Vulkan Context") << "Created physical device\n";
    }

    auto Context::createDevice() -> void
    {
        u32 propertyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertyCount, nullptr);
        std::vector<VkQueueFamilyProperties> properties(propertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &propertyCount, properties.data());

        for (u32 i = 0; i < propertyCount; ++i)
        {
            if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
                properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT &&
                glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, i))
            {
                graphicsQueue.family = i;
                graphicsQueue.index = 0;
                break;
            }
        }

        const f32 queuePriority = 1.f;

        VkDeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.pNext = nullptr;
        queueCreateInfo.flags = 0;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.queueFamilyIndex = graphicsQueue.family;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceVulkan11Features vulkan11Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        vulkan11Features.shaderDrawParameters     = true;

        VkPhysicalDeviceVulkan12Features vulkan12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        vulkan12Features.pNext = &vulkan11Features;
        vulkan12Features.runtimeDescriptorArray                             = true;
        vulkan12Features.descriptorBindingPartiallyBound                    = true;

        VkPhysicalDeviceVulkan13Features vulkan13Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        vulkan13Features.pNext = &vulkan12Features;
        vulkan13Features.synchronization2 = true;
        vulkan13Features.dynamicRendering = true;

        VkPhysicalDeviceFeatures2 enabledFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        enabledFeatures.pNext = &vulkan13Features;
        enabledFeatures.features.fillModeNonSolid        = true;
        enabledFeatures.features.multiDrawIndirect       = true;

        char const* extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

        VkDeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = (void*)&enabledFeatures;
        deviceCreateInfo.flags = 0;
        deviceCreateInfo.enabledExtensionCount = 1;
        deviceCreateInfo.ppEnabledExtensionNames = &extension;
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.pEnabledFeatures = nullptr;

        vkCheck(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
        volkLoadDevice(device);

        vkGetDeviceQueue(device, graphicsQueue.family, graphicsQueue.index, &graphicsQueue.queue);

        LOG(INFO, "Vulkan Context") << "Created device\n";
    }

    auto Context::createAllocator() -> void
    {
        VmaVulkanFunctions functions;
        functions.vkGetInstanceProcAddr                   = vkGetInstanceProcAddr;
        functions.vkGetDeviceProcAddr                     = vkGetDeviceProcAddr;
        functions.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
        functions.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
        functions.vkAllocateMemory                        = vkAllocateMemory;
        functions.vkFreeMemory                            = vkFreeMemory;
        functions.vkMapMemory                             = vkMapMemory;
        functions.vkUnmapMemory                           = vkUnmapMemory;
        functions.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
        functions.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
        functions.vkBindBufferMemory                      = vkBindBufferMemory;
        functions.vkBindImageMemory                       = vkBindImageMemory;
        functions.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
        functions.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
        functions.vkCreateBuffer                          = vkCreateBuffer;
        functions.vkDestroyBuffer                         = vkDestroyBuffer;
        functions.vkCreateImage                           = vkCreateImage;
        functions.vkDestroyImage                          = vkDestroyImage;
        functions.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
        functions.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2;
        functions.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2;
        functions.vkBindBufferMemory2KHR                  = vkBindBufferMemory2;
        functions.vkBindImageMemory2KHR                   = vkBindImageMemory2;
        functions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
        functions.vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements;
        functions.vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements;

        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorCreateInfo.physicalDevice = physicalDevice;
        allocatorCreateInfo.device = device;
        allocatorCreateInfo.instance = instance;
        allocatorCreateInfo.pVulkanFunctions = &functions;

        vkCheck(vmaCreateAllocator(&allocatorCreateInfo, &allocator));
        LOG(INFO, "Vulkan Context") << "Created allocator\n";
    }

    auto Context::teardown() -> void
    {
        vmaDestroyAllocator(allocator);

        if (device)
        {
            vkDestroyDevice(device, nullptr);
        }

        if (instance)
        {
            if (surface)       vkDestroySurfaceKHR(instance, surface, nullptr);
            if (debugCallback) vkDestroyDebugReportCallbackEXT(instance, debugCallback, nullptr);

            vkDestroyInstance(instance, nullptr);
        }
        
        LOG(INFO, "Vulkan Context") << "Destroyed context\n";
    }
}

static auto vkErrorString(VkResult result) -> char const*
{
    switch((i32)result)
    {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
        return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
    case VK_ERROR_INVALID_SHADER_NV:
        return "VK_ERROR_INVALID_SHADER_NV";
    default:
        return "VK_<Unknown>";
    }
}

static auto vkCheck(VkResult result) -> void
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("Vulkan error: ") + vkErrorString(result));
    }
}

static VkBool32 VKAPI_CALL debugReportCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    u64 object,
    size_t location,
    i32 messageCode,
    char const* pLayerPrefix,
    char const* pMessage,
    void* pUserData)
{
    if (VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT != flags)
    {
        LOG(DEBUG, "Validation Layer") << pMessage << '\n';
    }
    
    return VK_FALSE;
}
