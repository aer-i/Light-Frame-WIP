#include "Viewport.hpp"
#include "Renderer.hpp"

Viewport::Viewport(Renderer& renderer)
    : m{
        .renderer = renderer,
        .viewport = renderer.getViewportTexture(),
        .resolutionOption = 2
    }
{}

auto Viewport::render() -> void
{
    if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_MenuBar))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Resolution"))
            {
                auto clicked{ bool{} };

                clicked = clicked || ImGui::RadioButton("2160p", &m.resolutionOption, 0);
                clicked = clicked || ImGui::RadioButton("1440p", &m.resolutionOption, 1);
                clicked = clicked || ImGui::RadioButton("1080p", &m.resolutionOption, 2);
                clicked = clicked || ImGui::RadioButton("720p",  &m.resolutionOption, 3);
                clicked = clicked || ImGui::RadioButton("600p",  &m.resolutionOption, 4);
                clicked = clicked || ImGui::RadioButton("360p",  &m.resolutionOption, 5);

                if (clicked)
                {
                    switch (m.resolutionOption)
                    {
                    case 0: m.renderer.resizeViewport({3840, 2160}); break;
                    case 1: m.renderer.resizeViewport({2560, 1440}); break;
                    case 2: m.renderer.resizeViewport({1920, 1080}); break;
                    case 3: m.renderer.resizeViewport({1280, 720});  break;
                    case 4: m.renderer.resizeViewport({800, 600});   break;
                    case 5: m.renderer.resizeViewport({640, 360});   break;
                    default:
                        break;
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::Image(&m.viewport, ImGui::GetContentRegionAvail());
    }
    ImGui::End();
}
