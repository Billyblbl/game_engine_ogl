#ifndef GTRANSFORM
# define GTRANSFORM

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

struct Transform2D {
	glm::vec2 translation = glm::vec2(0);
	glm::vec2 scale = glm::vec2(1);
	float rotation = .0f;

	glm::mat4 matrix() const {
		return (
			glm::translate(glm::mat4(1), glm::vec3(translation, 0)) * // Translation
			glm::rotate(glm::mat4(1), glm::radians(rotation), glm::vec3(0,0,-1)) * // Rotation
			glm::scale(glm::mat4(1), glm::vec3(scale, 1)) // Scale;
		);
	}

};

struct OrthoCamera {
	glm::vec3	dimensions = glm::vec3(600, 400, 1);
	glm::vec3 center = glm::vec3(0);

	glm::mat4 matrix() const {
		auto min = -dimensions / 2.f - center;
		auto max = dimensions / 2.f - center;
		return glm::ortho(min.x, max.x, min.y, max.y, min.z, max.z);
	}
};

#endif
