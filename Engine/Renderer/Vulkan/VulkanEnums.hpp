#pragma once

namespace vk
{
    using ImageUsageFlags  = unsigned;
    using BufferUsageFlags = unsigned;
    using AspectFlags      = unsigned;
    using ShaderStageFlags = unsigned;

    enum class Format : unsigned
    {
        eR8_unorm          = 0x00000009,
        eRGBA8_unorm       = 0x00000025,
        eBGRA8_unorm       = 0x0000002C,
        eRG32_sfloat       = 0x00000067,
        eRGB32_sfloat      = 0x0000006A,
        eRGBA32_sfloat     = 0x0000006D,
        eD32_sfloat        = 0x0000007E,
        eD24_unorm_S8_uint = 0x00000081
    };

    enum class PresentMode : unsigned
    {
        eImmediate = 0x00000000,
        eMailbox   = 0x00000001,
        eFifo      = 0x00000002
    };

    enum class ImageLayout : unsigned
    {
        eUndefined                  = 0x00000000,
        eGeneral                    = 0x00000001,
        eColorAttachment            = 0x00000002,
        eDepthStencilAttachment     = 0x00000003,
        eDepthStencilRead           = 0x00000004,
        eShaderRead                 = 0x00000005,
        eTransferSrc                = 0x00000006,
        eTransferDst                = 0x00000007,
        eDepthReadStencilAttachment = 0x3B9C9308,
        eDepthAttachmentStencilRead = 0x3B9C9309,
        eDepthAttachment            = 0x3B9E7768,
        eDepthRead                  = 0x3B9E7769,
        eStencilAttachment          = 0x3B9E776A,
        eStencilRead                = 0x3B9E776B,
        eRead                       = 0x3B9F9490,
        eAttachment                 = 0x3B9F9491,
        ePresent                    = 0x3B9ACDEA
    };

    enum class DescriptorType : unsigned
    {
        eSampler              = 0x00000000,
        eCombinedImageSampler = 0x00000001,
        eSampledImage         = 0x00000002,
        eStorageImage         = 0x00000003,
        eUniformBuffer        = 0x00000006,
        eStorageBuffer        = 0x00000007
    };

    enum class MemoryType : unsigned
    {
        eHost     = 0x00000000,
        eHostOnly = 0x00000001,
        eDevice   = 0x00000002
    };

    namespace ImageUsage
    {
        enum : unsigned
        {
            eSampled         = 0x00000004,
            eStorage         = 0x00000008,
            eColorAttachment = 0x00000010,
            eDepthAttachment = 0x00000020
        };
    }

    namespace BufferUsage
    {
        enum : unsigned
        {
            eUniformBuffer  = 0x00000010,
            eStorageBuffer  = 0x00000020,
            eIndexBuffer    = 0x00000040,
            eIndirectBuffer = 0x00000100
        };
    };

    namespace Aspect
    {
        enum : unsigned
        {
            eColor   = 0x00000001,
            eDepth   = 0x00000002,
            eStencil = 0x00000004
        };
    }

    namespace ShaderStage
    {
        enum : unsigned
        {
            eVertex   = 0x00000001,
            eGeometry = 0x00000008,
            eFragment = 0x00000010,
            eCompute  = 0x00000020
        };
    }
}