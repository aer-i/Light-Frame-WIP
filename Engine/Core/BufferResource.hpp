#pragma once
#include <memory_resource>
#include <array>

namespace pmr
{
    inline std::array<std::uint8_t, 1024 * 512> g_stackBuffer;
    inline std::pmr::monotonic_buffer_resource g_rsrc(g_stackBuffer.data(), g_stackBuffer.size());
}
