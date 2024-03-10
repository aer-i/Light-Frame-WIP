#include "Editor.hpp"
#include <imgui.h>

Editor::Editor(Renderer& renderer)
    : m{
        .viewport = Viewport{ renderer }
    }
{}

auto Editor::render() -> void
{
    ImGui::ShowMetricsWindow();
    m.viewport.render();
}
