#ifndef GVERTEX
# define GVERTEX

#include <GL/glew.h>
#include <glutils.cpp>
#include <math.cpp>
#include <blblstd.hpp>
#include <buffer.cpp>

struct VertexAttributeSpecs {
	GLint member_count = 0;
	GLenum member_type = 0;
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

struct VertexArray {
	GLuint id;
	u32 element_count;
	GLenum index_type;
	GLenum draw_mode;
	Array<const VertexAttributeSpecs> layout;

	void bind_vertex_data(GLuint vbo, GLuint ibo, u32 count, u64 offset = 0) {
		GL_GUARD(glBindVertexArray(id));
		GL_GUARD(glBindBuffer(GL_ARRAY_BUFFER, vbo));
		GL_GUARD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
		for (GLuint i : u64xrange{ 0, layout.size() }) {
			GL_GUARD(glEnableVertexArrayAttrib(id, i));
			GL_GUARD(glVertexAttribPointer(i, layout[i].member_count, layout[i].member_type, GL_FALSE, layout[i].stride, (GLvoid*)(layout[i].offset + offset)));
		}
		GL_GUARD(glBindVertexArray(0));
		GL_GUARD(glBindBuffer(GL_ARRAY_BUFFER, 0));
		GL_GUARD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		for (GLuint i : u64xrange{ 0, layout.size() })
			GL_GUARD(glDisableVertexArrayAttrib(id, i));
		element_count = count;
	}

	static VertexArray create(Array<const VertexAttributeSpecs> vertex_attributes, GLuint draw_mode = GL_TRIANGLES, GLenum index_type = gl_type_table<u32>.upload_type) {
		VertexArray va;
		va.layout = vertex_attributes;
		va.draw_mode = draw_mode;
		va.index_type = index_type;
		GL_GUARD(glCreateVertexArrays(1, &va.id));
		return va;
	}

	void release() { GL_GUARD(glDeleteVertexArrays(1, &id)); }
};

#pragma region defaults

struct DefaultVertex2D {
	v2f32 position;
	v2f32 uv;
};

const VertexAttributeSpecs defaultVertex2DAttributes[] = {
	SpecsOf(DefaultVertex2D, position),
	SpecsOf(DefaultVertex2D, uv)
};

template<> const auto vertexAttributesOf<DefaultVertex2D> = larray(defaultVertex2DAttributes);

# pragma endregion defaults

#endif
