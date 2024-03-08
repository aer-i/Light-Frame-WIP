#pragma once

struct VkInstance_T;
struct VkDebugUtilsMessengerEXT_T;

using VkInstance = VkInstance_T*;
using VkDebugUtilsMessengerEXT = VkDebugUtilsMessengerEXT_T*;

namespace vk
{
    class Instance
    {
    public:
        Instance() = default;
        Instance(bool validationLayersEnabled);
        ~Instance();
        Instance(Instance&& other);
        Instance(Instance const&) = delete;
        auto operator=(Instance&& other) -> Instance&;
        auto operator=(Instance const&)  -> Instance& = delete;

    public:
        inline operator VkInstance() const noexcept
        {
            return m.instance;
        }

        inline auto apiVersion() const noexcept -> unsigned int
        {
            return m.apiVersion;
        }

    private:
        struct M
        {
            VkInstance               instance;
            VkDebugUtilsMessengerEXT debugMessenger;
            unsigned int             apiVersion;
        } m;
    };
}