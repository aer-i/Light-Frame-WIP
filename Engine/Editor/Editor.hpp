#pragma once
#include "Viewport.hpp"

class Renderer;

class Editor
{
public:
    Editor(Renderer& renderer);
    ~Editor() = default;
    Editor(Editor const&) = delete;
    Editor(Editor&&) = delete;
    auto operator=(Editor const&) -> Editor& = delete;
    auto operator=(Editor&&) -> Editor& = delete;

public:
    auto render() -> void;

private:
    struct M
    {
        Viewport viewport;
    } m;  
};