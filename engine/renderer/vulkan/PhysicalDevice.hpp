#pragma once

struct VkPhysicalDevice_T;

using VkPhysicalDevice = VkPhysicalDevice_T*;

namespace vk
{
    class Instance;

    class PhysicalDevice
    {
    public:
        PhysicalDevice(Instance& instance);
        ~PhysicalDevice() = default;
        PhysicalDevice(PhysicalDevice&& other);
        PhysicalDevice(PhysicalDevice const&) = delete;
        auto operator=(PhysicalDevice&& other) -> PhysicalDevice&;
        auto operator=(PhysicalDevice const&)  -> PhysicalDevice& = delete;

    public:
        inline operator VkPhysicalDevice() const noexcept
        {
            return m_physicalDevice;
        }

    private:
        VkPhysicalDevice m_physicalDevice;
    };
}
