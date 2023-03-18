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

GLuint create_shader(utf8 source, GLenum type) {
	auto shader = GL_GUARD(glCreateShader(type));
	const cstrp ptr = (const cstrp)source.data();
	const GLint len = static_cast<GLint>(source.size());
	GL_GUARD(glShaderSource(shader, 1, &ptr, &len));
	GL_GUARD(glCompileShader(shader));

	GLint is_compiled = 0;
	GL_GUARD(glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled));
	if (!is_compiled) {
		GLint log_length;
		GL_GUARD(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length));
		std::string log(log_length, '\0');
		GL_GUARD(glGetShaderInfoLog(shader, log_length, nullptr, log.data()));
		fprintf(stderr, "Failed to build shader - Shader log : %s\n", log.data());
		return 0;
	} else {
		return shader;
	}
}

GLuint load_shader(const char* path, GLenum type) {

	if (std::ifstream file{ path, std::ios::binary | std::ios::ate }) {
		usize size = file.tellg();
		std::string str(size, '\0');
		u8 buffer[size];
		file.seekg(0);
		file.read((cstrp)&buffer[0], size);
		file.close();
		return create_shader(utf8((const char8_t*)&buffer[0], size), type);
	} else {
		fprintf(stderr, "failed to open file %s\n", path);
		return 0;
	}

}

GLuint create_render_pipeline(GLuint vertex_shader, GLuint fragment_shader) {

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

struct GPUBinding {
	GLuint id;
	GLuint target;
	GLsizeiptr size;
};

void inline bind_texture(GPUBinding& binding) { GL_GUARD(glBindTextureUnit(binding.target, binding.id)); }
void inline bind_UBO(GPUBinding& binding) { GL_GUARD(glBindBufferRange(GL_UNIFORM_BUFFER, binding.target, binding.id, 0, binding.size)); }
void inline bind_SSBO(GPUBinding& binding) { GL_GUARD(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding.target, binding.id, 0, binding.size)); }
// GL_GUARD(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo.binding, ssbo.id));
void inline unbind_texture(GPUBinding& binding) { GL_GUARD(glBindTextureUnit(binding.target, 0)); }
void inline unbind_UBO(GPUBinding& binding) { GL_GUARD(glBindBufferRange(GL_UNIFORM_BUFFER, binding.target, 0, 0, binding.size)); }
void inline unbind_SSBO(GPUBinding& binding) { GL_GUARD(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding.target, 0, 0, binding.size)); }
// GL_GUARD(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo.binding, ssbo.id));

inline GPUBinding	bind(GLuint id, GLuint target, size_t size) { return GPUBinding{ id, target, (GLsizeiptr)size }; }
inline GPUBinding	bind(const Textures::Texture& mapping, GLuint target) { return GPUBinding{ mapping.id, target, mapping.dimensions.x * mapping.dimensions.y * mapping.channels }; }
template<typename T> inline GPUBinding	bind(const MappedObject<T>& mapping, GLuint target) { return GPUBinding{ mapping.id, target, sizeof(mapping.obj) }; }
template<typename T> inline GPUBinding	bind(const MappedBuffer<T>& mapping, GLuint target) { return GPUBinding{ mapping.id, target, (GLsizeiptr)mapping.obj.size_bytes() }; }

struct Material {
	GLuint renderProgram;
	std::span<GPUBinding>	textures;
};

struct RenderData {
	GLuint vao;
	Material* material;
};

template<typename Func> void render(GLuint fbf, rtu32 viewport, GLbitfield clear_flags, Func commands) {
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
	Array<GPUBinding> textures = {},
	Array<GPUBinding> ssbos = {},
	Array<GPUBinding> ubos = {}
) {
	GL_GUARD(glUseProgram(pipeline));
	for (auto&& texture : textures) bind_texture(texture);
	for (auto&& ubo : ubos) bind_UBO(ubo);
	for (auto&& ssbo : ssbos) bind_SSBO(ssbo);
	GL_GUARD(glBindVertexArray(mesh.vao));

	GL_GUARD(glDrawElementsInstanced(mesh.draw_mode, mesh.element_count, mesh.index_type, nullptr, instance_count));

	GL_GUARD(glBindVertexArray(0));
	for (auto&& ssbo : ssbos) unbind_SSBO(ssbo);
	for (auto&& ubo : ubos) unbind_UBO(ubo);
	for (auto&& texture : textures) unbind_texture(texture);
	GL_GUARD(glUseProgram(0));
}

void wait_gpu() {
	GL_GUARD(glFinish());
}

#endif
