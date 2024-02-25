#pragma once
#include <string>
#include <string_view>
#include "Renderer.hpp"
#include "Camera.hpp"
#include "ModelComponent.hpp"

class Scene
{
public:
    explicit Scene(std::string_view name) : m_name{ name }
    {
        m_camera.position = { 0, 0, -5 };
        m_camera.yawPitch.x = -90;
    }

public:
    Camera m_camera;
    MeshLoader m_meshLoader;
    ModelComponent m_model{ m_meshLoader, "assets/kitten.obj", false };
    vk::Buffer m_indirectDrawBuffer;
    std::string m_name;
};