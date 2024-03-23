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
    m.camera.update();
    m.camera.setProjection(70.f, 1920 / 1080, 0.1f, 1024.0f);
}
