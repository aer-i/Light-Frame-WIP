#pragma once
#include "Renderer.hpp"
#include <imgui.h>

namespace vk::imgui
{
    auto init()           -> void;
    auto teardown()       -> void;
    auto update()         -> void;
    auto render(Cmd& cmd) -> void;
}