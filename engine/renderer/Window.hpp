#pragma once
#include "Types.hpp"
#include <glm/glm.hpp>
#include <string>
#include <string_view>

struct SDL_Window;

class Window
{
public:
    static auto Create()   -> void;
    static auto Teardown() -> void;
    static auto Update()   -> void;
    
    static auto SetTitle(std::string_view title)  -> void;
    static auto SetRelativeMouseMode(bool enable) -> void;
    static auto GetRelativeMouseMode()            -> bool;
    static auto GetKey(i32 key)                   -> bool;
    static auto GetKeyDown(i32 key)               -> bool;
    static auto GetKeyUp(i32 key)                 -> bool;
    static auto GetButton(int button)             -> bool;
    static auto GetButtonDown(int button)         -> bool;
    static auto GetButtonUp(int button)           -> bool;
    static auto GetTime()                         -> f64;

    static inline auto GetDeltaTime()     -> const f32          { return m_deltaTime;   }
    static inline auto GetHandle()        -> const SDL_Window*  { return m_handle;      }
    static inline auto GetSize()          -> const glm::ivec2&  { return m_size;        }
    static inline auto GetWidth()         -> const i32          { return m_size.x;      }
    static inline auto GetHeight()        -> const i32          { return m_size.y;      }
    static inline auto GetTile()          -> const std::string& { return m_title;       }
    static inline auto GetPos()           -> const glm::ivec2&  { return m_pos;         }
    static inline auto GetPosX()          -> const i32          { return m_pos.x;       }
    static inline auto GetPosY()          -> const i32          { return m_pos.y;       }
    static inline auto GetCursorPos()     -> const glm::vec2&   { return m_cursorPos;   }
    static inline auto GetCursorX()       -> const f32          { return m_cursorPos.x; }
    static inline auto GetCursorY()       -> const f32          { return m_cursorPos.y; }
    static inline auto GetCursorOffset()  -> const glm::vec2&   { return m_cursorOff;   }
    static inline auto GetCursorOffsetX() -> const f32          { return m_cursorOff.x; }
    static inline auto GetCursorOffsetY() -> const f32          { return m_cursorOff.y; }
    static inline auto Available()        -> const bool         { return m_available;   }

private:
    static inline SDL_Window* m_handle        = nullptr;
    static inline glm::ivec2  m_size          = glm::ivec2(1280, 720);
    static inline glm::ivec2  m_pos           = glm::ivec2(50, 50);
    static inline glm::vec2   m_cursorPos     = glm::vec2(0, 0);
    static inline glm::vec2   m_cursorOff     = glm::vec2(0, 0);
    static inline glm::vec2   m_globCursorPos = glm::vec2(0, 0);
    static inline std::string m_title         = "Light Frame";
    static inline f32         m_deltaTime     = f32(0.f);
    static inline u8*         m_keyboardState = nullptr;
    static inline bool        m_available     = bool(true);
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