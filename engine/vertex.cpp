#ifndef GVERTEX
# define GVERTEX

#include <GL/glew.h>
#include <glutils.cpp>
#include <math.cpp>
#include <blblstd.hpp>
#include <buffer.cpp>
#include <pipeline.cpp>

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

	static VertexArray create(GLScope& ctx, GLuint draw_mode = GL_TRIANGLES, GLenum index_type = gl_type_table<u32>.upload_type) {
		VertexArray vao;
		vao.draw_mode = draw_mode;
		vao.index_type = index_type;
		GL_GUARD(glCreateVertexArrays(1, &vao.id));
		ctx.push<&GLScope::vaos>(vao.id);
		return vao;
	}

	GLint conf_vattrib(GLint attr, const VertexAttributeFormat& format) {
		GL_GUARD(glEnableVertexArrayAttrib(id, attr));
		switch (format.type) {
			case GL_DOUBLE: GL_GUARD(glVertexArrayAttribLFormat(id, attr, format.count, format.type, format.relative_offset));  break;
			case GL_INT: case GL_UNSIGNED_INT:
			case GL_SHORT: case GL_UNSIGNED_SHORT:
			case GL_BYTE: case GL_UNSIGNED_BYTE:
			GL_GUARD(glVertexArrayAttribIFormat(id, attr, format.count, format.type, format.relative_offset)); break;
			default: GL_GUARD(glVertexArrayAttribFormat(id, attr, format.count, format.type, format.normalised, format.relative_offset)); break;
		}
		return attr;
	}

	[[deprecated]] GLuint bind_vattrib(GLuint pipeline, const cstr name, GLuint binding, const VertexAttributeFormat& format) {
		auto attr = get_shader_input(pipeline, name, R_VERT);
		conf_vattrib(attr, format);
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

#endif
