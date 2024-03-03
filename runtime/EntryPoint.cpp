#include <iostream>
#include <spdlog/spdlog.h>
#include <SDL3/SDL.h>
#include "Engine.hpp"

auto main() -> i32
{
    try
    {
        auto engine{ Engine() };
        engine.execute();
    }
    catch (std::exception const& e)
    {
        spdlog::critical(e.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", e.what(), nullptr);
        return EXIT_FAILURE;
    }
}