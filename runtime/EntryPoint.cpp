#include <iostream>
#include <aixlog.hpp>
#include "Engine.hpp"

auto main() -> int
{
    try
    {
        Engine::Init();
        Engine::Execute();
        Engine::Teardown();
    }
    catch (std::exception const& e)
    {
        LOG(FATAL, "Exception") << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
