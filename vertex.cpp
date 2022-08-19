#ifndef GVERTEX
# define GVERTEX

#include <span>
#include <GL/glew.h>
#include <glutils.cpp>
#include <glm/glm/glm.hpp>

struct VertexAttributeSpecs {
	GLint memberCount = 0;
	GLenum memberType = 0;
	size_t offset;
};

template<typename T> constexpr auto MakeVertexAttributeSpec(size_t offset) { return VertexAttributeSpecs{}; };
template<> constexpr auto MakeVertexAttributeSpec<float>(size_t offset) { return VertexAttributeSpecs{ 1, GL_FLOAT, offset }; };
template<> constexpr auto MakeVertexAttributeSpec<glm::vec1>(size_t offset) { return VertexAttributeSpecs{ 1, GL_FLOAT, offset }; };
template<> constexpr auto MakeVertexAttributeSpec<glm::vec2>(size_t offset) { return VertexAttributeSpecs{ 2, GL_FLOAT, offset }; };
template<> constexpr auto MakeVertexAttributeSpec<glm::vec3>(size_t offset) { return VertexAttributeSpecs{ 3, GL_FLOAT, offset }; };
template<> constexpr auto MakeVertexAttributeSpec<glm::vec4>(size_t offset) { return VertexAttributeSpecs{ 4, GL_FLOAT, offset }; };
template<> constexpr auto MakeVertexAttributeSpec<int32_t>(size_t offset) { return VertexAttributeSpecs{ 1, GL_INT, offset }; };
template<> constexpr auto MakeVertexAttributeSpec<glm::ivec1>(size_t offset) { return VertexAttributeSpecs{ 1, GL_INT, offset }; };
template<> constexpr auto MakeVertexAttributeSpec<glm::ivec2>(size_t offset) { return VertexAttributeSpecs{ 2, GL_INT, offset }; };
template<> constexpr auto MakeVertexAttributeSpec<glm::ivec3>(size_t offset) { return VertexAttributeSpecs{ 3, GL_INT, offset }; };
template<> constexpr auto MakeVertexAttributeSpec<glm::ivec4>(size_t offset) { return VertexAttributeSpecs{ 4, GL_INT, offset }; };

#define SpecsOf(vertex, attribute) MakeVertexAttributeSpec<decltype(vertex::attribute)>(offsetof(vertex, attribute))

GLuint recordVAO(
	std::span<const VertexAttributeSpecs>	attributes,
	GLsizei stride,
	GLuint vbo,
	GLuint ibo
) {
	GLuint id;
	GL_GUARD(glCreateVertexArrays(1, &id));

	GL_GUARD(glBindVertexArray(id));
	GL_GUARD(glBindBuffer(GL_ARRAY_BUFFER, vbo));
	GL_GUARD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));

	for (GLuint i = 0; i < attributes.size(); i++) {
		auto& attribute = attributes[i];
		GL_GUARD(glEnableVertexAttribArray(i));
		GL_GUARD(glVertexAttribPointer(i, attribute.memberCount, attribute.memberType, GL_FALSE, stride, reinterpret_cast<GLvoid*>(attribute.offset)));
	}

	GL_GUARD(glBindVertexArray(0));
	GL_GUARD(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GL_GUARD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

	for (GLuint i = 0; i < attributes.size(); i++)
		GL_GUARD(glDisableVertexAttribArray(i));
	return id;
}

template<typename T> constexpr VertexAttributeSpecs vertexAttributesOf[] = {};

struct DefaultVertex2D {
	glm::vec2 position;
	glm::vec4 color;
};

template<> constexpr VertexAttributeSpecs vertexAttributesOf<DefaultVertex2D>[2] = {
	SpecsOf(DefaultVertex2D, position),
	SpecsOf(DefaultVertex2D, color)
};

#endif
