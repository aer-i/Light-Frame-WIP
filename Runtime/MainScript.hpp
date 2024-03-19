#pragma once
#include "LightFrameScript.hpp"

class MainScript : public lf::Script
{
    void onAwake() override;
    void onInit() override;
    void onUpdate() override;
    void onQuit() override;
};