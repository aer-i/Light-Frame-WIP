#pragma once
#include "Types.hpp"
#include "BufferResource.hpp"
#include <glm/glm.hpp>
#include <string>
#include <string_view>

struct SDL_Window;

class Window
{
public:
    Window();
    ~Window();
    Window(Window const&) = delete;
    Window(Window&&) = delete;
    auto operator=(Window const&) -> Window& = delete;
    auto operator=(Window&&) -> Window& = delete;

    auto update()                          -> void;
    auto setTitle(std::string_view title)  -> void;
    auto setCursorPos(glm::vec2 pos)       -> void;
    auto setRelativeMouseMode(bool enable) -> void;
    auto getRelativeMouseMode()            -> bool;
    auto getKey(i32 key)                   -> bool;
    auto getKeyDown(i32 key)               -> bool;
    auto getKeyUp(i32 key)                 -> bool;
    auto getButton(i32 button)             -> bool;
    auto getButtonDown(i32 button)         -> bool;
    auto getButtonUp(i32 button)           -> bool;
    auto getTime()                         -> f64;

    inline auto getDeltaTime()     -> const f32               { return m.deltaTime;   }
    inline auto getHandle()        -> const SDL_Window*       { return m.handle;      }
    inline auto getSize()          -> const glm::ivec2&       { return m.size;        }
    inline auto getWidth()         -> const i32               { return m.size.x;      }
    inline auto getHeight()        -> const i32               { return m.size.y;      }
    inline auto getTile()          -> const std::pmr::string& { return m.title;       }
    inline auto getPos()           -> const glm::ivec2&       { return m.pos;         }
    inline auto getPosX()          -> const i32               { return m.pos.x;       }
    inline auto getPosY()          -> const i32               { return m.pos.y;       }
    inline auto getCursorPos()     -> const glm::vec2&        { return m.cursorPos;   }
    inline auto getCursorX()       -> const f32               { return m.cursorPos.x; }
    inline auto getCursorY()       -> const f32               { return m.cursorPos.y; }
    inline auto getCursorOffset()  -> const glm::vec2&        { return m.cursorOff;   }
    inline auto getCursorOffsetX() -> const f32               { return m.cursorOff.x; }
    inline auto getCursorOffsetY() -> const f32               { return m.cursorOff.y; }
    inline auto available()        -> const bool              { return m.available;   }

private:
    struct M
    {
        SDL_Window*      handle;
        glm::ivec2       size;
        glm::ivec2       pos;
        glm::vec2        cursorPos;
        glm::vec2        cursorOff;
        glm::vec2        globCursorPos;
        std::pmr::string title;
        f32              deltaTime;
        u8*              keyboardState;
        bool             available;
    } m;
};

namespace key
{
    enum : i32
    {
        eA            = 4,
        eB            = 5,
        eC            = 6,
        eD            = 7,
        eE            = 8,
        eF            = 9,
        eG            = 10,
        eH            = 11,
        eI            = 12,
        eJ            = 13,
        eK            = 14,
        eL            = 15,
        eM            = 16,
        eN            = 17,
        eO            = 18,
        eP            = 19,
        eQ            = 20,
        eR            = 21,
        eS            = 22,
        eT            = 23,
        eU            = 24,
        eV            = 25,
        eW            = 26,
        eX            = 27,
        eY            = 28,
        eZ            = 29,
        e1            = 30,
        e2            = 31,
        e3            = 32,
        e4            = 33,
        e5            = 34,
        e6            = 35,
        e7            = 36,
        e8            = 37,
        e9            = 38,
        e0            = 39,
        eReturn       = 40,
        eEscape       = 41,
        eBackspace    = 42,
        eTab          = 43,
        eSpace        = 44,
        eMinus        = 45,
        eEquals       = 46,
        eLeftBracket  = 47,
        eRightBracket = 48,
        eBackSlash    = 49,
        eSemicolon    = 51,
        eApostrophe   = 52,
        eGrave        = 53,
        eComma        = 54,
        ePeriod       = 55,
        eSlash        = 56,
        eCapslock     = 57,
        eF1           = 58,
        eF2           = 59,
        eF3           = 60,
        eF4           = 61,
        eF5           = 62,
        eF6           = 63,
        eF7           = 64,
        eF8           = 65,
        eF9           = 66,
        eF10          = 67,
        eF11          = 68,
        eF12          = 69,
        ePrintScreen  = 70,
        eScrollLock   = 71,
        ePause        = 72,
        eInsert       = 73,
        eHome         = 74,
        ePageUp       = 75,
        eDelete       = 76,
        eEnd          = 77,
        ePageDown     = 78,
        eRight        = 79,
        eLeft         = 80,
        eDown         = 81,
        eUp           = 82,
        eLeftCtrl     = 224,
        eLeftShift    = 225,
        eLeftAlt      = 226,
        eLeftGui      = 227,
        eRightCtrl    = 228,
        eRightShift   = 229,
        eRightAlt     = 230,
        eRightGui     = 231
    };
}

namespace button
{
    enum : i32
    {
        eNone   = 0,
        eLeft   = 1,
        eMiddle = 2,
        eRight  = 3,
        eX1     = 4,
        eX2     = 5
    };
}