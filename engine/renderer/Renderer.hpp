#pragma once
#include "Types.hpp"
#include "ArrayProxy.hpp"
#include <string>
#include <functional>

struct VkImage_T;
struct VkImageView_T;
struct VkBuffer_T;
struct VmaAllocation_T;
struct VmaAllocationInfo;

using VkImage = VkImage_T*;
using VkBuffer = VkBuffer_T*;
using VkImageView = VkImageView_T*;
using VmaAllocation = VmaAllocation_T*;

namespace vk
{
    using flags = u32;
    using ImageUsageFlags = flags;
    using AspectFlags = flags;
    using ShaderStageFlags = flags;
    using BufferUsageFlags = flags;

    namespace ImageUsage
    {
        enum 
        {
            eTransferSrc = 0x00000001,
            eTransferDst = 0x00000002,
            eSampled = 0x00000004,
            eStorage = 0x00000008,
            eColorAttachment = 0x00000010,
            eDepthAttachment = 0x00000020,
        };
    }

    namespace Aspect
    {
        enum
        {
            eColor = 1,
            eDepth = 2,
            eStencil = 4,
        };
    }

    namespace ShaderStage
    {
        enum
        {
            eVertex = 0x00000001,
            eGeometry = 0x00000008,
            eFragment = 0x00000010,
            eCompute = 0x00000020
        };
    }

    namespace BufferUsage
    {
        enum
        {
            eTransferSrc = 0x00000001,
            eTransferDst = 0x00000002,
            eUniformBuffer = 0x00000010,
            eStorageBuffer = 0x00000020,
            eIndirectBuffer = 0x00000100
        };
    };

    enum class Format : flags
    {
        eRGBA8_unorm = 37,
        eBGRA8_unorm = 44,
        eRG32_sfloat = 103,
        eRGB32_sfloat = 106,
        eRGBA32_sfloat = 109,
        eD32_sfloat = 126,
        eD24_unorm_S8_uint = 129
    };

    enum class ImageLayout : flags
    {
        eUndefined = 0,
        eGeneral = 1,
        eColorAttachment = 2,
        eDepthStencilAttachment = 3,
        eDepthStencilRead = 4,
        eShaderRead = 5,
        eTransferSrc = 6,
        eTransferDst = 7,
        eDepthReadStencilAttachment = 1000117000,
        eDepthAttachmentStencilRead = 1000117001,
        eDepthAttachment = 1000241000,
        eDepthRead = 1000241001,
        eStencilAttachment = 1000241002,
        eStencilRead = 1000241003,
        eRead = 1000314000,
        eAttachment = 1000314001,
        ePresent = 1000001002,
    };

    enum class PipelineBindPoint : flags
    {
        eGraphics = 0,
        eCompute = 1
    };

    enum class MemoryType : flags
    {
        eHost = 0x0,
        eHostOnly = 0x1,
        eDevice = 0x2
    };

    enum class DescriptorType : flags
    {
        eSampler = 0,
        eCombinedImageSampler = 1,
        eSampledImage = 2,
        eStorageImage = 3,
        eUniformBuffer = 6,
        eStorageBuffer = 7
    };

    class Image
    {
    public:
        Image() = default;
        ~Image();

        auto loadFromSwapchain(VkImage image, VkImageView imageView, Format imageFormat) -> void;

        VkImage handle;
        VkImageView handleView;
        VmaAllocation allocation;
        ImageUsageFlags usage;
        AspectFlags aspect;
        ImageLayout layout;
        Format format;
        u32 width;
        u32 height;
        u32 layerCount;
    };

    class Buffer
    {
    public:
        Buffer() = default;
        ~Buffer();

        auto allocate(u32 dataSize, BufferUsageFlags usage, MemoryType memory) -> void;
        auto writeData(void const* data) -> void;

        void* mappedData;
        VkBuffer handle;
        VmaAllocation allocation;
        u32 memoryType;
        u32 size;
    };

    struct PipelineShaderStage
    {
        ShaderStageFlags stage;
        std::string      filepath;
    };

    struct PipelineDescriptor
    {
        u32 binding;
        ShaderStageFlags shaderStage;
        DescriptorType descriptorType;
        Buffer* pBuffer;
    };

    struct Pipeline
    {
        ~Pipeline();

        u32               handleIndex;
        u32               descriptor;
        PipelineBindPoint bindPoint;
    };

    struct PipelineConfig
    {
        PipelineBindPoint               bindPoint;
        ArrayProxy<PipelineShaderStage> stages;
        ArrayProxy<PipelineDescriptor>  descriptors;
        bool                            depthWrite;
        bool                            colorBlending;
    };

    auto init()                                          -> void;
    auto teardown()                                      -> void;
    auto acquire()                                       -> void;
    auto present()                                       -> void;
    auto waitIdle()                                      -> void;
    auto cmdBegin()                                      -> void;
    auto cmdEnd()                                        -> void;
    auto cmdBeginPresent()                               -> void;
    auto cmdEndPresent()                                 -> void;
    auto cmdBeginRendering(Image const& image)           -> void;
    auto cmdEndRendering()                               -> void;
    auto cmdBarrier(Image& image, ImageLayout newLayout) -> void;
    auto cmdBindPipeline(Pipeline& pipeline)             -> void;
    auto cmdDraw(u32 vertexCount)                        -> void;
    auto cmdNext()                                       -> void;
    auto onResize(std::function<void()> resizeCallback)  -> void;
    auto createPipeline(PipelineConfig const& config)    -> Pipeline;
    auto getCommandBufferCount()                         -> u32;
}