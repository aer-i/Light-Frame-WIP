#pragma once
#include "BufferResource.hpp"
#include "LightFrameScript.hpp"
#include "Window.hpp"
#include "Renderer.hpp"
#include "Editor.hpp"
#include <string_view>
#include <memory>

namespace lf
{
    template<typename T>
    class Engine
    {
    public:
        Engine() = default;
        ~Engine() = default;
        Engine(Engine&&) = delete;
        Engine(Engine const&) = delete;
        auto operator=(Engine&&) -> Engine& = delete;
        auto operator=(Engine const&) -> Engine& = delete;

    public:
        auto execute() -> void
        {
            for (auto& script : m.scripts)
            {
                script.second->onAwake();
            }

            static_cast<T*>(this)->registerScripts();

            while (m.window.available()) [[likely]]
            {
                ImGui::ShowDemoWindow();

                if (ImGui::Begin("Scripts"), ImGuiWindowFlags_Popup)
                {
                    static char const* currentItem = nullptr;

                    if (ImGui::BeginCombo("##combo", currentItem))
                    {
                        for (auto& script : m.scripts)
                        {
                            bool isSelected = currentItem == script.first.c_str();
                            if (ImGui::Selectable(script.first.c_str(), isSelected))
                            {
                                currentItem = script.first.c_str();
                            }

                            if (isSelected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }

                        ImGui::EndCombo();
                    }
                }
                ImGui::End();

                m.window.update();
                m.editor.render();
                m.renderer.renderFrame();
            }

            m.renderer.waitIdle();

            for (auto& script : m.scripts)
            {
                script.second->onQuit();
            }
        }

    protected:
        template<typename ScriptClass>
        auto registerScript(std::string_view name)
        {
            m.scripts[std::pmr::string{name.data(), &pmr::g_rsrc}] = std::make_unique<ScriptClass>();
        }

    private:
        using ScriptMap = std::pmr::unordered_map<std::pmr::string, std::unique_ptr<Script>>;

        struct M
        {
            Window    window   = Window{ };
            Renderer  renderer = Renderer{ window };
            Editor    editor   = Editor{ renderer };
            ScriptMap scripts  = ScriptMap{ &pmr::g_rsrc };
        } m;
    };
}