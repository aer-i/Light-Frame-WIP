#pragma once
#include "Types.hpp"
#include "ArrayProxy.hpp"
#include <string>

struct VkImage_T;
struct VkImageView_T;
struct VmaAllocation_T;

using VkImage = VkImage_T*;
using VkImageView = VkImageView_T*;
using VmaAllocation = VmaAllocation_T*;

namespace vk
{
    using flags = u32;
    using ImageUsageFlags = flags;
    using AspectFlags = flags;
    using ShaderStageFlags = flags;

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

    class Image
    {
    public:
        Image() = default;
        ~Image();

        auto loadFromSwapchain(VkImage image, VkImageView imageView, Format format) -> void;

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

    struct PipelineShaderStage
    {
        ShaderStageFlags stage;
        std::string      filepath;
    };

    struct Pipeline
    {
        u32               handleIndex;
        PipelineBindPoint bindPoint;
    };

    struct PipelineConfig
    {
        PipelineBindPoint               bindPoint;
        ArrayProxy<PipelineShaderStage> stages;
        bool                            depthWrite;
        bool                            colorBlending;
    };

    auto init()                                          -> void;
    auto teardown()                                      -> void;
    auto acquire()                                       -> void;
    auto present()                                       -> void;
    auto cmdBegin()                                      -> void;
    auto cmdEnd()                                        -> void;
    auto cmdBeginPresent()                               -> void;
    auto cmdEndPresent()                                 -> void;
    auto cmdBeginRendering(Image const& image)           -> void;
    auto cmdEndRendering()                               -> void;
    auto cmdBarrier(Image& image, ImageLayout newLayout) -> void;
    auto cmdBindPipeline(Pipeline& pipeline)             -> void;
    auto cmdDraw(u32 vertexCount)                        -> void;
    auto createPipeline(PipelineConfig const& config)    -> Pipeline;
}