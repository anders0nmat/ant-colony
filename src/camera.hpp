#pragma once

#include <glm/glm.hpp>

class Camera {
public:
	glm::vec3 pos;
	glm::vec2 size;

	Camera(glm::vec3 pos, glm::vec2 size) : pos(pos), size(size) {}

	void zoom(float factor);
	void move(glm::vec2 direction);

	glm::vec2 viewArea() const;

	glm::mat4 getProjectionMatrix() const;
	glm::mat4 getViewMatrix() const;
};