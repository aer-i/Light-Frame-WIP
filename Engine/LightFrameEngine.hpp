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

            auto time{ f32{} };
            auto fps{ u32{} };

            while (m.window.available()) [[likely]]
            {
                if (time <= 1)
                {
                    time += m.window.getDeltaTime();
                    ++fps;
                }
                else 
                {
                    m.window.setTitle(std::to_string(fps));
                    time = 0;
                    fps = 0;
                }

                m.renderer.renderFrame();
                m.window.update();
                m.editor.render();
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