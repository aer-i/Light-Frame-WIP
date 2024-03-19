#include "Engine.hpp"
#include "EntryPoint.hpp"
#include "MainScript.hpp"

int main(int argc, char** argv)
{
    return lf::Main<Engine>(argc, argv);
}

void Engine::registerScripts()
{
    this->registerScript<MainScript>("Main");
}
