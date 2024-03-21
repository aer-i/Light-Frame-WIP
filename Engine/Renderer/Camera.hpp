#pragma once
#include <glm/glm.hpp>

class Camera
{
public:
    inline auto getProjection() -> glm::mat4 const&
    {
        return m.projection;
    }

    inline auto getView() -> glm::mat4 const&
    {
        return m.view;
    }

    inline auto getProjectionView() -> glm::mat4
    {
        return m.projection * m.view;
    }

protected:
    struct M
    {
        glm::mat4 projection = glm::mat4{ 1.0f };
        glm::mat4 view       = glm::mat4{ 1.0f };
    } m;
};