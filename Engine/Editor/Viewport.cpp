#include "Viewport.hpp"
#include "Renderer.hpp"

Viewport::Viewport(Renderer& renderer)
    : m{
        .renderer = renderer,
        .camera = EditorCamera{ renderer.getWindow() }
    }
{
    renderer.setCamera(&m.camera);
}

auto Viewport::render() -> void
{
    auto const screenSize{ m.renderer.getWindow().getSize() };

    m.camera.setProjection(70.f, static_cast<f32>(screenSize.x) / static_cast<f32>(screenSize.y), 0.1f, 1024.0f);
    m.camera.update();
}
