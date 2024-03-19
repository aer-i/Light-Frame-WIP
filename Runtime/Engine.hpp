#pragma once
#include "LightFrameEngine.hpp"

class Engine : public lf::Engine<Engine>
{
public:
    void registerScripts();
};