#ifndef GRENDERING
# define GRENDERING

#include <GL/glew.h>
#include <glutils.cpp>
#include <fstream>
#include <buffer.cpp>
#include <textures.cpp>
#include <model.cpp>
#include <math.cpp>
#include <framebuffer.cpp>

//TODO remove fstream dependency

GLuint create_shader(str source, GLenum type) {
	auto shader = GL_GUARD(glCreateShader(type));

	const cstrp content[] = {
		"#version 450 core\n"
		"#define ", GLtoString(type).substr(3).data(),"\n\n", // shader type macro
		source.data()
	};
	const auto size = array_size(content);

	GL_GUARD(glShaderSource(shader, size, content, null));
	GL_GUARD(glCompileShader(shader));

	GLint is_compiled = 0;
	GL_GUARD(glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled));
	if (!is_compiled) {
		GLint log_length;
		GL_GUARD(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length));
		GLchar log[log_length + 1];
		memset(log, 0, log_length + 1);
		GL_GUARD(glGetShaderInfoLog(shader, log_length, nullptr, log));
		fprintf(stderr, "Failed to build shader - Shader log : %s\n", log);
		return 0;
	} else {
		return shader;
	}
}

GLuint load_shader(const char* path, GLenum type) {
	printf("Loading shader %s\n", path);
	fflush(stdout);
	if (std::ifstream file{ path, std::ios::binary | std::ios::ate }) {
		usize size = file.tellg();
		char buffer[size + 1] = { 0 };
		file.seekg(0);
		file.read(buffer, size);
		file.close();
		return create_shader(str(buffer, size), type);
	} else {
		fprintf(stderr, "failed to open file %s\n", path);
		return 0;
	}

}

GLuint create_render_pipeline(GLuint vertex_shader, GLuint fragment_shader) {
	if (vertex_shader == 0 || fragment_shader == 0) {
		fprintf(stderr, "Failed to build render pipeline, invalid shader\n");
		return 0;
	}

	auto program = GL_GUARD(glCreateProgram());
	GL_GUARD(glAttachShader(program, vertex_shader));
	GL_GUARD(glAttachShader(program, fragment_shader));
	GL_GUARD(glLinkProgram(program));
	GLint linked;
	GL_GUARD(glGetProgramiv(program, GL_LINK_STATUS, &linked));
	GL_GUARD(glDetachShader(program, vertex_shader));
	GL_GUARD(glDetachShader(program, fragment_shader));
	GL_GUARD(glDeleteShader(vertex_shader));
	GL_GUARD(glDeleteShader(fragment_shader));

	if (!linked) {
		GLint logLength;
		GL_GUARD(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength));
		char log[logLength + 1];
		log[logLength] = '\0';
		GL_GUARD(glGetProgramInfoLog(program, logLength, nullptr, log));
		fprintf(stderr, "Failed to build render pipeline %u, pipeline program log : %s\n", program, log);
		return 0;
	} else
		return program;
}

GLuint load_pipeline(const char* path) {
	printf("Loading pipeline %s\n", path);
	fflush(stdout);
	if (std::ifstream file{ path, std::ios::binary | std::ios::ate }) {
		usize size = file.tellg();
		char buffer[size + 1] = { 0 };
		file.seekg(0);
		file.read(buffer, size);
		file.close();
		return create_render_pipeline(
			create_shader(str(buffer, size), GL_VERTEX_SHADER),
			create_shader(str(buffer, size), GL_FRAGMENT_SHADER)
		);
	} else {
		fprintf(stderr, "failed to open file %s\n", path);
		return 0;
	}

}

struct GPUBinding {
	enum { None = 0, Texture, UBO, SSBO } type;
	GLuint id;
	GLuint target;
	GLsizeiptr size;
};

void inline bind(const GPUBinding& binding) {
	switch (binding.type) {
	case GPUBinding::Texture: GL_GUARD(glBindTextureUnit(binding.target, binding.id)); break;
	case GPUBinding::UBO: GL_GUARD(glBindBufferRange(GL_UNIFORM_BUFFER, binding.target, binding.id, 0, binding.size)); break;
	case GPUBinding::SSBO: GL_GUARD(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding.target, binding.id, 0, binding.size)); break;
		//TODO better error handling
	default: assert(false); break;
	}
}

void inline unbind(const GPUBinding& binding) {
	switch (binding.type) {
	case GPUBinding::Texture: GL_GUARD(glBindTextureUnit(binding.target, 0)); break;
	case GPUBinding::UBO: GL_GUARD(glBindBufferRange(GL_UNIFORM_BUFFER, binding.target, 0, 0, binding.size)); break;
	case GPUBinding::SSBO: GL_GUARD(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding.target, 0, 0, binding.size)); break;
		//TODO better error handling
	default: assert(false); break;
	}
}

inline GPUBinding bind_to(GLuint id, GLuint target, size_t size) { return GPUBinding{ GPUBinding::None, id, target, (GLsizeiptr)size }; }
inline GPUBinding bind_to(const Texture& mapping, GLuint target) { return GPUBinding{ GPUBinding::Texture, mapping.id, target, mapping.dimensions.x * mapping.dimensions.y * mapping.channels }; }
template<typename T> inline GPUBinding bind_to(const MappedObject<T>& mapping, GLuint target) { return GPUBinding{ GPUBinding::UBO, mapping.id, target, sizeof(mapping.obj) }; }
template<typename T> inline GPUBinding bind_to(const MappedBuffer<T>& mapping, GLuint target) { return GPUBinding{ GPUBinding::SSBO, mapping.id, target, (GLsizeiptr)mapping.obj.size_bytes() }; }

template<typename Func> inline void render(GLuint fbf, rtu32 viewport, GLbitfield clear_flags, Func commands) {
	GL_GUARD(glBindFramebuffer(GL_FRAMEBUFFER, fbf));
	GL_GUARD(glViewport(viewport.min.x, viewport.min.x, width(viewport), height(viewport)));
	GL_GUARD(glClear(clear_flags));
	commands();
	GL_GUARD(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void draw(
	GLuint pipeline,
	const RenderMesh& mesh,
	u32 instance_count = 1,
	Array<const GPUBinding> bindings = {}
) {
	GL_GUARD(glUseProgram(pipeline));
	GL_GUARD(glBindVertexArray(mesh.vao));
	for (auto&& binding : bindings) bind(binding);
	GL_GUARD(glDrawElementsInstanced(mesh.draw_mode, mesh.element_count, mesh.index_type, nullptr, instance_count));
	for (auto&& binding : bindings) unbind(binding);
	GL_GUARD(glBindVertexArray(0));
	GL_GUARD(glUseProgram(0));
}

void draw(
	GLuint pipeline,
	const RenderMesh& mesh,
	u32 instance_count,
	LiteralArray<GPUBinding> bindings
) {
	draw(pipeline, mesh, instance_count, larray(bindings));
}

void wait_gpu() {
	GL_GUARD(glFinish());
}

#endif
