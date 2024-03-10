#pragma once
#include "Renderer.hpp"
#include "Types.hpp"
#include <glm/glm.hpp>
#include <map>
#include <string_view>
#include <vector>

class Font
{
public:
    Font(std::string_view path);

private:
    struct Character
    {
        f32 advanceX;
        f32 advanceY;
        f32 width;
        f32 height;
        f32 left;
        f32 top;
        f32 offsetX;
    };

public:
    std::map<char, Character> characters;
    glm::vec2 atlasSize;
    vk::Image texture;
};