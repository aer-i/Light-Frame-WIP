#pragma once

namespace vk
{
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
        eImmediate = 0,
        eMailbox = 1,
        eFifo = 2
    };
}