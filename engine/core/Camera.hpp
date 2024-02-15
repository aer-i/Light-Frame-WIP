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
		static auto showCursor{ bool{true} };

		auto velocity{ glm::vec3{} };
		auto speed{ f32{5.f} };

		if (Window::GetKey(SDL_SCANCODE_W)) velocity -= front;
		if (Window::GetKey(SDL_SCANCODE_S)) velocity += front;
		if (Window::GetKey(SDL_SCANCODE_D)) velocity += right;
		if (Window::GetKey(SDL_SCANCODE_A)) velocity -= right;
		if (Window::GetKey(SDL_SCANCODE_E)) velocity.y += 1.f;
		if (Window::GetKey(SDL_SCANCODE_Q)) velocity.y -= 1.f;
		if (Window::GetKey(SDL_SCANCODE_LSHIFT)) speed = 10.f;

		if (velocity != glm::vec3{})
		{
			position += normalize(velocity) * Window::GetDeltaTime() * speed;
		}

		if (showCursor)
		{
			yawPitch.x = glm::mod(yawPitch.x - Window::GetCursorOffsetX() * 0.1f, 360.f);
			yawPitch.y += Window::GetCursorOffsetY() * 0.1f;
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
		projection = { glm::mat4{} };
		projection[0][0] = 1.f / (aspect * thf);
		projection[1][1] = 1.f / (thf);
		projection[2][2] = far / (far - near);
		projection[2][3] = 1.f;
		projection[3][2] = -(far * near) / (far - near);
	}

	auto setView(glm::vec3 const& direction) -> void
	{
		auto const w{ glm::normalize(direction) };
		auto const u{ glm::normalize(cross(w, { 0.f, 1.f, 0.f })) };
		auto const v{ glm::cross(u, w) };
		view = { glm::mat4{1.f} };
		view[0][0] = u.x;
		view[1][0] = u.y;
		view[2][0] = u.z;
		view[0][1] = v.x;
		view[1][1] = v.y;
		view[2][1] = v.z;
		view[0][2] = -w.x;
		view[1][2] = -w.y;
		view[2][2] = -w.z;
		view[3][0] = -glm::dot(u, position);
		view[3][1] = -glm::dot(v, position);
    	view[3][2] =  glm::dot(w, position);
	}

public:
	glm::mat4 projection{ glm::mat4{1.0f} };
	glm::mat4 view		{ glm::mat4{1.0f} };
    glm::vec3 position  { glm::vec3{0.0f} };
    glm::vec3 front  	{ glm::vec3{0.0f} };
    glm::vec3 right  	{ glm::vec3{0.0f} };
    glm::vec3 up  	 	{ glm::vec3{0.0f} };
	glm::vec2 yawPitch	{ glm::vec2{0.0f} };
};