#ifndef GVERTEX
# define GVERTEX

#include <GL/glew.h>
#include <glutils.cpp>
#include <math.cpp>
#include <blblstd.hpp>

struct VertexAttributeSpecs {
	GLint memberCount = 0;
	GLenum memberType = 0;
	size_t offset = 0;
	GLsizei stride = 0;
};

template<typename T> constexpr auto make_vertex_attribute_spec(usize offset, GLsizei stride) { return VertexAttributeSpecs{ 0, 0, offset, stride }; };
template<> constexpr auto make_vertex_attribute_spec<float>(usize offset, GLsizei stride) { return VertexAttributeSpecs{ 1, GL_FLOAT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_spec<glm::vec1>(usize offset, GLsizei stride) { return VertexAttributeSpecs{ 1, GL_FLOAT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_spec<glm::vec2>(usize offset, GLsizei stride) { return VertexAttributeSpecs{ 2, GL_FLOAT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_spec<glm::vec3>(usize offset, GLsizei stride) { return VertexAttributeSpecs{ 3, GL_FLOAT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_spec<glm::vec4>(usize offset, GLsizei stride) { return VertexAttributeSpecs{ 4, GL_FLOAT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_spec<int32_t>(usize offset, GLsizei stride) { return VertexAttributeSpecs{ 1, GL_INT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_spec<glm::ivec1>(usize offset, GLsizei stride) { return VertexAttributeSpecs{ 1, GL_INT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_spec<glm::ivec2>(usize offset, GLsizei stride) { return VertexAttributeSpecs{ 2, GL_INT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_spec<glm::ivec3>(usize offset, GLsizei stride) { return VertexAttributeSpecs{ 3, GL_INT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_spec<glm::ivec4>(usize offset, GLsizei stride) { return VertexAttributeSpecs{ 4, GL_INT, offset, stride }; };

#define SpecsOf(vertex, attribute) make_vertex_attribute_spec<decltype(vertex::attribute)>(offsetof(vertex, attribute), sizeof(vertex))

// template<typename T> constexpr VertexAttributeSpecs vertexAttributesArray[] = {};
template<typename T> const Array<const VertexAttributeSpecs> vertexAttributesOf;

// TODO find why "assert(vertexAttributesOf<T>.size() > 0); // No Attributes defined" always fails even with DefaultVertex2D
// template<typename T> GLuint recordVAO(GLuint vbo, GLuint ibo) {
// 	fprintf(stdout, "vertex attributes %u\n", vertexAttributesOf<T>.size());
// 	fprintf(stdout, "vertex type %s\n", typeid(T).name() );
// 	fflush(stdout);
// 	assert(vertexAttributesOf<T>.size() > 0); // No Attributes defined
// 	// assert(vertexAttributesOf<T> == vertexAttributesOf<DefaultVertex2D>); // Type is DefaultVertex2d
// 	return recordVAO(vertexAttributesOf<T>, sizeof(T), vbo, ibo);
// }

GLuint record_VAO(
	Array<const VertexAttributeSpecs>	attributes,
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
		GL_GUARD(glVertexAttribPointer(i, attribute.memberCount, attribute.memberType, GL_FALSE, attribute.stride, reinterpret_cast<GLvoid*>(attribute.offset)));
	}

	GL_GUARD(glBindVertexArray(0));
	GL_GUARD(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GL_GUARD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

	for (GLuint i = 0; i < attributes.size(); i++)
		GL_GUARD(glDisableVertexAttribArray(i));
	return id;
}

#pragma region defaults

struct DefaultVertex2D {
	glm::vec2 position;
	glm::vec2 uv;
};

const VertexAttributeSpecs defaultVertex2DAttributes[] = {
	SpecsOf(DefaultVertex2D, position),
	SpecsOf(DefaultVertex2D, uv)
};

template<> const auto vertexAttributesOf<DefaultVertex2D> = larray(defaultVertex2DAttributes);

# pragma endregion defaults

#endif
