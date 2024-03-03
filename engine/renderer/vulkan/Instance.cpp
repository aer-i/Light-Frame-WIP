#include "Instance.hpp"
#include "Types.hpp"
#include <volk.h>
#include <SDL3/SDL_vulkan.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <cstdio>
#include <stdexcept>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* pUserData
)
{
    spdlog::debug(pCallbackData->pMessage);
    
    return VK_FALSE;
}

vk::Instance::Instance(bool validationLayersEnabled)
    : m_instance{ nullptr }
    , m_debugMessenger{ nullptr }
    , m_apiVersion{ 0u }
{
    if (volkInitialize())
    {
        throw std::runtime_error("Failed to initialize Volk");
    }

    spdlog::info("Loaded Vulkan functions");

    auto extensionCount{ u32{} };
    auto extensionNames{ SDL_Vulkan_GetInstanceExtensions(&extensionCount) };

    auto extensions{ std::vector<char const*>{extensionNames, extensionNames + extensionCount} };
    auto layers    { std::vector<char const*>{} };

    if (vkEnumerateInstanceVersion(&m_apiVersion))
    {
        throw std::runtime_error("Failed to enumerate instance version");
    }

    if (validationLayersEnabled)
    {
        spdlog::set_level(spdlog::level::debug);
        layers.emplace_back("VK_LAYER_KHRONOS_validation");
        extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    auto const syncValidationFeature{ VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT };

    auto const validationFeatures{ VkValidationFeaturesEXT{
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .enabledValidationFeatureCount = 1,
        .pEnabledValidationFeatures = &syncValidationFeature
    }};

    auto const applicationInfo{ VkApplicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Light Frame Application",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "Light Frame",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion = m_apiVersion
    }};

    auto const instanceCreateInfo{ VkInstanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = validationLayersEnabled ? &validationFeatures : nullptr,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<u32>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<u32>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    }};

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance))
    {
        throw std::runtime_error("Failed to create VkInstance");
    }

    volkLoadInstanceOnly(m_instance);

    if (validationLayersEnabled)
    {
        auto const debugMessengerCreateInfo{ VkDebugUtilsMessengerCreateInfoEXT{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = &debugCallback
        }};

        if (vkCreateDebugUtilsMessengerEXT(m_instance, &debugMessengerCreateInfo, nullptr, &m_debugMessenger))
        {
            throw std::runtime_error("Failed to create debug utils messenger");
        }
    }

    spdlog::info("Created instance");
    spdlog::info(
        "Instance API version [ {}.{}.{} ]",
        VK_VERSION_MAJOR(m_apiVersion),
        VK_VERSION_MINOR(m_apiVersion),
        VK_VERSION_PATCH(m_apiVersion)
    );
    spdlog::info("Enabled instance extenstions:");

    for (auto const& extension : extensions)
    {
        spdlog::info("   {}", extension);
    }

    spdlog::info("Enabled instance layers:");

    for (auto const& layer : layers)
    {
        spdlog::info("   {}", layer);
    }
}

vk::Instance::~Instance()
{
    if (m_debugMessenger)
    {
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        m_debugMessenger = nullptr;
    }

    if (m_instance)
    {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = nullptr;
    }

    spdlog::info("Destroyed instance");
}

vk::Instance::Instance(Instance&& other)
    : m_instance{ other.m_instance }
    , m_debugMessenger{ other.m_debugMessenger }
    , m_apiVersion{ other.m_apiVersion }
{
    other.m_debugMessenger = nullptr;
    other.m_instance       = nullptr;
    other.m_apiVersion     = 0;
}

auto vk::Instance::operator=(Instance&& other) -> Instance&
{
    this->m_instance       = other.m_instance;
    this->m_debugMessenger = other.m_debugMessenger;
    this->m_apiVersion     = other.m_apiVersion;

    other.m_debugMessenger = nullptr;
    other.m_instance       = nullptr;
    other.m_apiVersion     = 0;

    return *this;
}