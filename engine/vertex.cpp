#ifndef GVERTEX
# define GVERTEX

#include <GL/glew.h>
#include <glutils.cpp>
#include <math.cpp>
#include <blblstd.hpp>
#include <buffer.cpp>

// struct [[deprecated]] VertexAttributeLayout {
// 	GLint member_count = 0;
// 	GLenum member_type = 0;
// 	size_t offset = 0;
// 	GLsizei stride = 0;
// };

// template<typename T> constexpr auto make_vertex_attribute_layout(usize offset, GLsizei stride) { return VertexAttributeLayout{ 0, 0, offset, stride }; };
// template<> constexpr auto make_vertex_attribute_layout<f32>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 1, GL_FLOAT, offset, stride }; };
// template<> constexpr auto make_vertex_attribute_layout<glm::vec1>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 1, GL_FLOAT, offset, stride }; };
// template<> constexpr auto make_vertex_attribute_layout<glm::vec2>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 2, GL_FLOAT, offset, stride }; };
// template<> constexpr auto make_vertex_attribute_layout<glm::vec3>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 3, GL_FLOAT, offset, stride }; };
// template<> constexpr auto make_vertex_attribute_layout<glm::vec4>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 4, GL_FLOAT, offset, stride }; };
// template<> constexpr auto make_vertex_attribute_layout<i32>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 1, GL_INT, offset, stride }; };
// template<> constexpr auto make_vertex_attribute_layout<glm::ivec1>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 1, GL_INT, offset, stride }; };
// template<> constexpr auto make_vertex_attribute_layout<glm::ivec2>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 2, GL_INT, offset, stride }; };
// template<> constexpr auto make_vertex_attribute_layout<glm::ivec3>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 3, GL_INT, offset, stride }; };
// template<> constexpr auto make_vertex_attribute_layout<glm::ivec4>(usize offset, GLsizei stride) { return VertexAttributeLayout{ 4, GL_INT, offset, stride }; };

// #define SpecsOf(vertex, attribute) make_vertex_attribute_layout<decltype(vertex::attribute)>(offsetof(vertex, attribute), sizeof(vertex))
// #define UniqueAttr(vertex) make_vertex_attribute_layout<vertex>(0, sizeof(vertex))

// using GeometryLayout = Array<const VertexAttributeLayout>;
// template<typename T> const GeometryLayout vertexAttributesOf;

// [[deprecated]] void assemble_vao(GLuint vao, GLuint vbo, GLuint ibo, GeometryLayout layout, u64 vertex_offset = 0) {
// 	GL_GUARD(glBindVertexArray(vao));
// 	GL_GUARD(glBindBuffer(GL_ARRAY_BUFFER, vbo));
// 	GL_GUARD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
// 	for (GLuint i : u64xrange{ 0, layout.size() }) {
// 		GL_GUARD(glEnableVertexArrayAttrib(vao, i));
// 		GL_GUARD(glVertexAttribPointer(i, layout[i].member_count, layout[i].member_type, GL_FALSE, layout[i].stride, (GLvoid*)(layout[i].offset + vertex_offset)));
// 	}
// 	GL_GUARD(glBindVertexArray(0));
// 	GL_GUARD(glBindBuffer(GL_ARRAY_BUFFER, 0));
// 	GL_GUARD(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
// }

struct VertexAttributeFormat {
	GLint count;
	GLenum type;
	GLboolean normalised;
	GLuint relative_offset;
};

template<typename T> constexpr auto vattr_fmt(GLuint offset, GLboolean normalised = GL_FALSE) {
	return VertexAttributeFormat{
		.count = Format<T>.channel_count,
		.type = Format<T>.type,
		.normalised = normalised,
		.relative_offset = offset
	};
};

using VertexFormat = Array<const tuple<const cstr, VertexAttributeFormat>>;
using VertexInput = Array<const VertexFormat>;

struct VertexArray {
	GLuint id;
	GLenum index_type;
	GLenum draw_mode;

	// [[deprecated]] VertexArray& associate(GLuint vbo, GLuint ibo, GeometryLayout layout) { assemble_vao(id, vbo, ibo, layout); return *this; }

	static VertexArray create(GLScope& ctx, GLuint draw_mode = GL_TRIANGLES, GLenum index_type = gl_type_table<u32>.upload_type) {
		VertexArray vao;
		vao.draw_mode = draw_mode;
		vao.index_type = index_type;
		GL_GUARD(glCreateVertexArrays(1, &vao.id));
		ctx.push<&GLScope::vaos>(vao.id);
		return vao;
	}

	GLuint  bind_vattrib(GLuint pipeline, const cstr name, GLuint binding, const VertexAttributeFormat& format) {
		auto attr = GL_GUARD(glGetAttribLocation(pipeline, name));
		GL_GUARD(glEnableVertexArrayAttrib(id, attr));
		switch (format.type) {
			case GL_DOUBLE: GL_GUARD(glVertexArrayAttribLFormat(id, attr, format.count, format.type, format.relative_offset));  break;
			case GL_INT: case GL_UNSIGNED_INT:
			case GL_SHORT: case GL_UNSIGNED_SHORT:
			case GL_BYTE: case GL_UNSIGNED_BYTE:
			GL_GUARD(glVertexArrayAttribIFormat(id, attr, format.count, format.type, format.relative_offset)); break;
			default: GL_GUARD(glVertexArrayAttribFormat(id, attr, format.count, format.type, format.normalised, format.relative_offset)); break;
		}
		GL_GUARD(glVertexArrayAttribBinding(id, attr, binding));
		return binding;
	};

	GLuint bind_vertex_buffer(GLuint vbo, GLuint binding, GLintptr offset, GLsizei stride, GLuint divisor) {
		GL_GUARD(glVertexArrayVertexBuffer(id, binding, vbo, offset, stride));
		GL_GUARD(glVertexArrayBindingDivisor(id, binding, divisor));
		return binding;
	}

	void bind_index_buffer(GLuint ibo) {
		GL_GUARD(glVertexArrayElementBuffer(id, ibo));
	}

	void bind() const { GL_GUARD(glBindVertexArray(id)); }
	static void unbind() { GL_GUARD(glBindVertexArray(0)); }

};

#pragma region defaults

// struct DefaultVertex2D {
// 	v2f32 position;
// 	v2f32 uv;
// };

// const VertexAttributeLayout defaultVertex2DAttributes[] = {
// 	SpecsOf(DefaultVertex2D, position),
// 	SpecsOf(DefaultVertex2D, uv)
// };

// const VertexAttributeLayout v2f32Attributes[] = { UniqueAttr(v2f32) };

// template<> const GeometryLayout vertexAttributesOf<v2f32> = larray(v2f32Attributes);
// template<> const GeometryLayout vertexAttributesOf<DefaultVertex2D> = larray(defaultVertex2DAttributes);

# pragma endregion defaults

#endif
