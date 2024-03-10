#pragma once
#include "Types.hpp"

class Renderer;

class Viewport
{
public:
    Viewport(Renderer& renderer);
    ~Viewport() = default;
    Viewport(Viewport const&) = delete;
    Viewport(Viewport&&) = delete;
    auto operator=(Viewport const&) -> Viewport& = delete;
    auto operator=(Viewport&&) -> Viewport& = delete;

public:
    auto render() -> void;

private:
    struct M
    {
        Renderer& renderer;
        u32       viewport;
        i32       resolutionOption;
    } m;
};