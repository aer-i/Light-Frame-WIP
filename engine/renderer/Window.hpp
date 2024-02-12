#pragma once
#include "Types.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <string_view>

class Window
{
public:
    static auto Create()   -> void;
    static auto Teardown() -> void;
    static auto Update()   -> void;
    
    static auto IsKeyPressed(i32 key)            -> bool;
    static auto IsKeyDown(i32 key)               -> bool;
    static auto IsKeyUp(i32 key)                 -> bool;
    static auto SetTitle(std::string_view title) -> void;

    static inline auto GetTime()   -> const f64          { return glfwGetTime(); }
    static inline auto GetDelta()  -> const f32          { return m_deltaTime;   }
    static inline auto GetHandle() -> const GLFWwindow*  { return m_handle;      }
    static inline auto GetSize()   -> const glm::ivec2&  { return m_size;        }
    static inline auto GetWidth()  -> const i32          { return m_size.x;      }
    static inline auto GetHeight() -> const i32          { return m_size.y;      }
    static inline auto GetTile()   -> const std::string& { return m_title;       }
    static inline auto GetPos()    -> const glm::ivec2&  { return m_pos;         }
    static inline auto GetPosX()   -> const i32          { return m_pos.x;       }
    static inline auto GetPosY()   -> const i32          { return m_pos.y;       }
    static inline auto Available() -> const bool         { return m_available;   }

private:
    static auto FramebufferResizeCallback(GLFWwindow* window, i32 width, i32 height) -> void;
    static auto PositionChangeCallback(GLFWwindow* window, i32 x, i32 y) -> void;

private:
    static inline GLFWwindow* m_handle    = nullptr;
    static inline glm::ivec2  m_size      = glm::ivec2(1280, 720);
    static inline glm::ivec2  m_pos       = glm::ivec2(50, 50);
    static inline std::string m_title     = "Light Frame";
    static inline f32         m_deltaTime = f32(0.f);
    static inline bool        m_available = bool(true);
};