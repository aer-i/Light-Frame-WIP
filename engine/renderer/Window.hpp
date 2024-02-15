#pragma once
#include "Types.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <string>
#include <string_view>

class Window
{
public:
    static auto Create()   -> void;
    static auto Teardown() -> void;
    static auto Update()   -> void;
    
    static auto SetTitle(std::string_view title)  -> void;
    static auto GetKey(i32 key)                   -> bool;
    static auto GetKeyDown(i32 key)               -> bool;
    static auto GetKeyUp(i32 key)                 -> bool;
    static auto GetButton(int button)             -> bool;
    static auto GetButtonDown(int button)         -> bool;
    static auto GetButtonUp(int button)           -> bool;
    static auto GetTime()                         -> f64;

    inline void setRelativeMouseMode(bool t_enable = true) { SDL_SetRelativeMouseMode(t_enable); }
    inline auto getRelativeMouseMode() -> bool { return static_cast<bool>(SDL_GetRelativeMouseMode()); }

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
    static inline SDL_Event   m_event         = SDL_Event{};
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