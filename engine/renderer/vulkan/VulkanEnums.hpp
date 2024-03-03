#pragma once

namespace vk
{
    using ImageUsageFlags = unsigned;
    using AspectFlags     = unsigned;

    enum class Format : unsigned
    {
        eR8_unorm          = 9,
        eRGBA8_unorm       = 37,
        eBGRA8_unorm       = 44,
        eRG32_sfloat       = 103,
        eRGB32_sfloat      = 106,
        eRGBA32_sfloat     = 109,
        eD32_sfloat        = 126,
        eD24_unorm_S8_uint = 129
    };

    enum class PresentMode : unsigned
    {
        eImmediate = 0x00000000,
        eMailbox   = 0x00000001,
        eFifo      = 0x00000002
    };

    enum class ImageLayout : unsigned
    {
        eUndefined                  = 0,
        eGeneral                    = 1,
        eColorAttachment            = 2,
        eDepthStencilAttachment     = 3,
        eDepthStencilRead           = 4,
        eShaderRead                 = 5,
        eTransferSrc                = 6,
        eTransferDst                = 7,
        eDepthReadStencilAttachment = 1000117000,
        eDepthAttachmentStencilRead = 1000117001,
        eDepthAttachment            = 1000241000,
        eDepthRead                  = 1000241001,
        eStencilAttachment          = 1000241002,
        eStencilRead                = 1000241003,
        eRead                       = 1000314000,
        eAttachment                 = 1000314001,
        ePresent                    = 1000001002
    };

    namespace ImageUsage
    {
        enum : unsigned
        {
            eTransferSrc     = 0x00000001,
            eTransferDst     = 0x00000002,
            eSampled         = 0x00000004,
            eStorage         = 0x00000008,
            eColorAttachment = 0x00000010,
            eDepthAttachment = 0x00000020
        };
    }

    namespace Aspect
    {
        enum : unsigned
        {
            eColor   = 0x00000001,
            eDepth   = 0x00000002,
            eStencil = 0x00000004
        };
    }
}