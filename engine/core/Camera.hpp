#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <cmath>
#include "Types.hpp"
#include "Window.hpp"

class Camera
{
public:
    auto update() -> void
    {
        auto const v{ glm::mat4_cast(orientation) };

        auto const forward{ -glm::vec3(v[0][2], v[1][2], v[2][2]) };
		auto const right  {  glm::vec3(v[0][0], v[1][0], v[2][0]) };
		auto const up     {  glm::cross(right, forward)           };

        auto accel{ glm::vec3{} };
		auto sprint{ bool{} };

        if (Window::IsKeyPressed(GLFW_KEY_W)) accel += forward;
		if (Window::IsKeyPressed(GLFW_KEY_S)) accel -= forward;
		if (Window::IsKeyPressed(GLFW_KEY_A)) accel -= right;
		if (Window::IsKeyPressed(GLFW_KEY_D)) accel += right;
		if (Window::IsKeyPressed(GLFW_KEY_E)) accel += up;
		if (Window::IsKeyPressed(GLFW_KEY_Q)) accel -= up;

		if (Window::IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
		{
			accel *= fastCoef;
			sprint = true;
		}

		if (accel == glm::vec3(0))
		{
			moveSpeed -= moveSpeed * std::min((1.0f / damping) * static_cast<f32>(Window::GetDelta()), 1.0f);
		}
		else
		{
			moveSpeed += accel * acceleration * static_cast<f32>(Window::GetDelta());
			const auto limit{ sprint ? maxSpeed * fastCoef : maxSpeed };

			if (glm::length(moveSpeed) > limit)
            {
                moveSpeed = glm::normalize(moveSpeed) * limit;
            }
		}

		position += moveSpeed * static_cast<f32>(Window::GetDelta());
    }

    auto getView() -> glm::mat4
    {
        auto const t{ glm::translate(glm::mat4(1.0f), -position) };
		auto const r{ glm::mat4_cast(orientation) };
		return r * t;
    }

public:
	f32 acceleration{ 150.f };
	f32 maxSpeed    { 10.0f };
	f32 fastCoef    { 10.0f };
    f32 mouseSpeed  {  4.0f };
	f32 damping     {  0.1f };

public:
    glm::quat orientation{ glm::quat{ glm::vec3{} }  };
    glm::vec3 up         { glm::vec3{ 0.f, 1.f, 0.f} };
    glm::vec3 position   { glm::vec3{}               };
    glm::vec3 moveSpeed  { glm::vec3{}               };
};