#include "Editor.hpp"

Editor::Editor(Renderer& renderer)
    : m{
        .viewport = Viewport{ renderer }
    }
{}

auto Editor::render() -> void
{
    m.viewport.render();
}
