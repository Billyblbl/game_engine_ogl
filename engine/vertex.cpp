#ifndef GVERTEX
# define GVERTEX

#include <GL/glew.h>
#include <glutils.cpp>
#include <math.cpp>
#include <blblstd.hpp>
#include <buffer.cpp>

struct VertexAttributeLayout {
	GLint member_count = 0;
	GLenum member_type = 0;
	size_t offset = 0;
	GLsizei stride = 0;
	//TODO add "normalised" bool
};

template<typename T> constexpr auto make_vertex_attribute_layout(usize offset, GLsizei stride) { return VertexAttributeLayout{ 0, 0, offset, stride }; };
template<> constexpr auto make_vertex_attribute_layout<f32>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 1, GL_FLOAT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_layout<glm::vec1>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 1, GL_FLOAT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_layout<glm::vec2>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 2, GL_FLOAT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_layout<glm::vec3>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 3, GL_FLOAT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_layout<glm::vec4>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 4, GL_FLOAT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_layout<i32>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 1, GL_INT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_layout<glm::ivec1>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 1, GL_INT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_layout<glm::ivec2>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 2, GL_INT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_layout<glm::ivec3>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 3, GL_INT, offset, stride }; };
template<> constexpr auto make_vertex_attribute_layout<glm::ivec4>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 4, GL_INT, offset, stride }; };

#define SpecsOf(vertex, attribute) make_vertex_attribute_layout<decltype(vertex::attribute)>(offsetof(vertex, attribute), sizeof(vertex))

using GeometryLayout = Array<const VertexAttributeLayout>;
template<typename T> const GeometryLayout vertexAttributesOf;

void assemble_vao(GLuint vao, GLuint vbo, GLuint ibo, GeometryLayout layout, u64 vertex_offset = 0) {
	GL_GUARD(glBindVertexArray(vao));
	GL_GUARD(glBindBuffer(GL_ARRAY_BUFFER, vbo));
	GL_GUARD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
	for (GLuint i : u64xrange{ 0, layout.size() }) {
		GL_GUARD(glEnableVertexArrayAttrib(vao, i));
		GL_GUARD(glVertexAttribPointer(i, layout[i].member_count, layout[i].member_type, GL_FALSE, layout[i].stride, (GLvoid*)(layout[i].offset + vertex_offset)));
	}
	GL_GUARD(glBindVertexArray(0));
	GL_GUARD(glBindBuffer(GL_ARRAY_BUFFER, 0));
	GL_GUARD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

struct VertexArray {
	GLuint id;
	u32 element_count;
	GLenum index_type;
	GLenum draw_mode;

	VertexArray& associate(GLuint vbo, GLuint ibo, GeometryLayout layout) { assemble_vao(id, vbo, ibo, layout); return *this; }

	static VertexArray create(GLuint draw_mode = GL_TRIANGLES, GLenum index_type = gl_type_table<u32>.upload_type) {
		VertexArray va;
		va.draw_mode = draw_mode;
		va.index_type = index_type;
		va.element_count = 0;
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

const VertexAttributeLayout defaultVertex2DAttributes[] = {
	SpecsOf(DefaultVertex2D, position),
	SpecsOf(DefaultVertex2D, uv)
};

template<> const GeometryLayout vertexAttributesOf<DefaultVertex2D> = larray(defaultVertex2DAttributes);

# pragma endregion defaults

#endif
