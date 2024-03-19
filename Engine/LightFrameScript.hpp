#pragma once

namespace lf
{
    class Script
    {
    public:
        Script() = default;
        ~Script() = default;
        Script(Script const&) = delete;
        Script(Script&&) = delete;
        auto operator=(Script const&) -> Script& = delete;
        auto operator=(Script&&) -> Script& = delete;

    public:
        virtual auto onAwake()  -> void { };
        virtual auto onInit()   -> void { };
        virtual auto onUpdate() -> void { };
        virtual auto onQuit()   -> void { };
    };
}