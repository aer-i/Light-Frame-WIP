#pragma once
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <SDL3/SDL.h>

namespace lf
{
    template<typename Engine>
    auto Main(int argc, char** argv) -> int
    {
        try
        {
            auto engine{ Engine{} };
            engine.execute();
        }
        catch (std::exception const& e)
        {
            spdlog::critical(e.what());
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", e.what(), nullptr);
            return EXIT_FAILURE;
        }
        
        return EXIT_SUCCESS;
    }
}