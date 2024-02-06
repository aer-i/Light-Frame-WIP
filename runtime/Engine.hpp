#pragma once

class Engine
{
public:
    static auto Init()     -> void;
    static auto Execute()  -> void;
    static auto Teardown() -> void;
};