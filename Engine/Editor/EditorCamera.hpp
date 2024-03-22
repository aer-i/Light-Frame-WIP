#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "Camera.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <cmath>
#include "Types.hpp"
#include "Window.hpp"

class EditorCamera : public Camera
{
public:
	EditorCamera() = default;
	EditorCamera(Window& window)
		: window{ &window }
	{}

    auto update() -> void
    {
		auto velocity{ glm::vec3{} };
		auto speed{ f32{5.f} };

		if (window->getKey(key::eW)) velocity -= front;
		if (window->getKey(key::eS)) velocity += front;
		if (window->getKey(key::eD)) velocity += right;
		if (window->getKey(key::eA)) velocity -= right;
		if (window->getKey(key::eE)) velocity.y += 1.f;
		if (window->getKey(key::eQ)) velocity.y -= 1.f;
		if (window->getKey(key::eLeftShift)) speed = 15.f;

		if (velocity != glm::vec3{})
		{
			position += normalize(velocity) * window->getDeltaTime() * speed;
		}

        if (window->getButtonDown(button::eRight))
        {
            mousePos = window->getCursorPos();
            window->setRelativeMouseMode(true);
        }

		if (window->getButton(button::eRight))
		{
            window->setCursorPos(mousePos);

			yawPitch.x = glm::mod(yawPitch.x - window->getCursorOffsetX() * 0.1f, 360.f);
			yawPitch.y += window->getCursorOffsetY() * 0.1f;
		}

        if (window->getButtonUp(button::eRight))
        {
            window->setRelativeMouseMode(false);
            window->setCursorPos(mousePos);
        }

		if (yawPitch.y >  89.9f) yawPitch.y =  89.9f;
    	if (yawPitch.y < -89.9f) yawPitch.y = -89.9f;

		front = glm::normalize(glm::vec3{
				std::cos(glm::radians(yawPitch.x)) * std::cos(glm::radians(yawPitch.y)),
				std::sin(glm::radians(yawPitch.y)),
				std::sin(glm::radians(yawPitch.x)) * std::cos(glm::radians(yawPitch.y))
			}
		);
		right = glm::normalize(glm::cross(front, { 0.f, 1.f, 0.f }));
		up    = glm::normalize(glm::cross(right, front));

		this->setView(front);
    }

	auto setProjection(f32 fovY, f32 aspect, f32 near, f32 far) -> void
	{
		auto const thf{ static_cast<f32>(std::tan(fovY / 2.f)) };
		m.projection = { glm::mat4{} };
		m.projection[0][0] = 1.f / (aspect * thf);
		m.projection[1][1] = 1.f / (thf);
		m.projection[2][2] = far / (far - near);
		m.projection[2][3] = 1.f;
		m.projection[3][2] = -(far * near) / (far - near);
	}

	auto setView(glm::vec3 const& direction) -> void
	{
		auto const w{ glm::normalize(direction) };
		auto const u{ glm::normalize(cross(w, { 0.f, 1.f, 0.f })) };
		auto const v{ glm::cross(u, w) };
		m.view = { glm::mat4{1.f} };
		m.view[0][0] = u.x;
		m.view[1][0] = u.y;
		m.view[2][0] = u.z;
		m.view[0][1] = v.x;
		m.view[1][1] = v.y;
		m.view[2][1] = v.z;
		m.view[0][2] = -w.x;
		m.view[1][2] = -w.y;
		m.view[2][2] = -w.z;
		m.view[3][0] = -glm::dot(u, position);
		m.view[3][1] = -glm::dot(v, position);
    	m.view[3][2] =  glm::dot(w, position);
	}

private:
	Window*   window   { nullptr };
    glm::vec3 position { glm::vec3{0.0f} };
    glm::vec3 front    { glm::vec3{0.0f} };
    glm::vec3 right    { glm::vec3{0.0f} };
    glm::vec3 up  	   { glm::vec3{0.0f} };
	glm::vec2 yawPitch { glm::vec2{0.0f} };
    glm::vec2 mousePos { glm::vec2{0.0f} };
};