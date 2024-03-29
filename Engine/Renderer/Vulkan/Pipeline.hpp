#pragma once
#include "VulkanEnums.hpp"
#include "ArrayProxy.hpp"
#include <string>

struct VkPipelineLayout_T;
struct VkPipeline_T;
struct VkDescriptorSetLayout_T;
struct VkDescriptorSet_T;

using VkPipelineLayout      = VkPipelineLayout_T*;
using VkPipeline            = VkPipeline_T*;
using VkDescriptorSetLayout = VkDescriptorSetLayout_T*;
using VkDescriptorSet       = VkDescriptorSet_T*;

namespace vk
{
    class Device;
    class Buffer;
    class SwapBuffer;
    class Image;

    class Pipeline
    {
    public:
        enum class BindPoint : u32
        {
            eGraphics = 0x00000000,
            eCompute  = 0x00000001
        };

        enum class Topology : u32
        {
            ePoint         = 0x00000000,
            eLineList      = 0x00000001,
            eLineStrip     = 0x00000002,
            eTriangleList  = 0x00000003,
            eTriangleStrip = 0x00000004,
            eTriangleFan   = 0x00000005
        };

        enum class CullMode : u32
        {
            eNone  = 0x00000000,
            eFront = 0x00000001,
            eBack  = 0x00000002
        };

        struct ShaderStage
        {
            ShaderStageFlags stage;
            std::string      path;
        };

        struct Descriptor
        {
            u32              binding;
            ShaderStageFlags stage;
            DescriptorType   type;
            Buffer*          pBuffer;
            SwapBuffer*      pSwapBuffer;
            size_t           offset;
            size_t           size;
        };

        struct Config
        {
            BindPoint               point;
            ArrayProxy<ShaderStage> stages;
            ArrayProxy<Descriptor>  descriptors;
            Topology                topology;
            CullMode                cullMode;
            bool                    depthWrite;
            bool                    depthTest;
            bool                    colorBlending;
            bool                    usePushConstant;
        };

    public:
        Pipeline();
        Pipeline(Device& device, Config const& config);
        ~Pipeline();
        Pipeline(Pipeline const&) = delete;
        Pipeline(Pipeline&& other);
        auto operator=(Pipeline const&)  -> Pipeline& = delete;
        auto operator=(Pipeline&& other) -> Pipeline&;

    public:
        auto writeImage(Image& image, u32 element, DescriptorType type) -> void;

    public:
        inline operator VkPipeline() const noexcept
        {
            return m.pipeline;
        }

        inline operator VkPipelineLayout() const noexcept
        {
            return m.layout;
        }

        template<typename T>
        inline auto operator()(T frameIndex) const noexcept -> VkDescriptorSet
        {
            return m.sets.empty() ? nullptr : m.sets[frameIndex];
        }

        inline auto getBindPoint() const noexcept
        {
            return m.point;
        }

    private:
        struct M
        {
            std::vector<VkDescriptorSet> sets;
            Device*                      device;
            VkPipelineLayout             layout;
            VkPipeline                   pipeline;
            VkDescriptorSetLayout        setLayout;
            BindPoint                    point;
            u32                          imagesBinding;
        } m;
    };
}