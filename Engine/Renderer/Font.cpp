#include "Font.hpp"
#include <stdexcept>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <aixlog.hpp>

struct FreeTypeRAII
{
    FreeTypeRAII()
    {
        FT_Init_FreeType(&ft);
    }

    ~FreeTypeRAII()
    {
        FT_Done_FreeType(ft);
    }

    FT_Library ft;
};

auto static g_ftRaii{ FreeTypeRAII{} };

Font::Font(std::string_view path)
{
    FT_Face face;

    if (FT_New_Face(g_ftRaii.ft, path.data(), 0, &face))
    {
        throw std::runtime_error(std::string("Failed to load font: ") + path.data());
    }

    FT_Set_Pixel_Sizes(face, 0, 250);

    auto* glyph{ face->glyph };
    auto width{ u32{} }, height{ u32{} };

    for (auto c{ u8{' '} }; c < 128; ++c)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            LOG(WARNING) << "Failed to load character: " << c << '\n';
            continue;
        }

        width += glyph->bitmap.width;
        height = std::max(height, glyph->bitmap.rows);
    }

    atlasSize = glm::vec2{ width, height };

    texture.allocate({ width, height }, vk::ImageUsage::eSampled, vk::Format::eR8_unorm);

    auto offsetX{ u32{} };

    for (auto c{ u8{' '} }; c < 128; ++c)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            continue;
        }

        auto character{ Character{
            .advanceX = static_cast<f32>(glyph->advance.x >> 6),
            .advanceY = static_cast<f32>(glyph->advance.y >> 6),
            .width    = static_cast<f32>(glyph->bitmap.width),
            .height   = static_cast<f32>(glyph->bitmap.rows),
            .left     = static_cast<f32>(glyph->bitmap_left),
            .top      = static_cast<f32>(glyph->bitmap_top),
            .offsetX  = static_cast<f32>(offsetX) / static_cast<f32>(width),
        }};

        characters.insert(std::pair<char, Character>(c, character));

        if (glyph->bitmap.width && glyph->bitmap.rows)
        {
            const auto writeSize{ size_t{glyph->bitmap.width * glyph->bitmap.rows} };
            texture.subwrite(glyph->bitmap.buffer, writeSize, { offsetX, 0 }, { glyph->bitmap.width, glyph->bitmap.rows });

            offsetX += glyph->bitmap.width;
        }
    }

    FT_Done_Face(face);
}