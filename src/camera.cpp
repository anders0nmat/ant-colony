#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

void Camera::zoom(float factor) {
	size *= factor;
}

void Camera::move(glm::vec2 direction) {
	pos.x += direction.x;
	pos.y += direction.y;
}

glm::vec2 Camera::viewArea() const {
	return size + glm::vec2(pos.x, pos.y);
}

glm::mat4 Camera::getProjectionMatrix() const {
	const glm::vec2 half_size = size * 0.5f;
	return glm::ortho(-half_size.x, half_size.x, -half_size.y, half_size.y, 0.1f, 10.0f);
}

glm::mat4 Camera::getViewMatrix() const {
	return glm::lookAt(pos, glm::vec3(pos.x, pos.y, 0), glm::vec3(0, 1, 0));
}
