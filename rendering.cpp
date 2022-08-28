#ifndef GRENDERING
# define GRENDERING

#include <GL/glew.h>
#include <glutils.cpp>
#include <fstream>
#include <string_view>
#include <span>
#include <buffer.cpp>
#include <textures.cpp>

GLuint createShader(std::string_view source, GLenum type) {
	auto shader = GL_GUARD(glCreateShader(type));
	const auto ptr = source.data();
	const GLint len = static_cast<GLint>(source.size());
	GL_GUARD(glShaderSource(shader, 1, &ptr, &len));
	GL_GUARD(glCompileShader(shader));

	GLint isCompiled = 0;
	GL_GUARD(glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled));
	if (!isCompiled) {
		GLint logLength;
		GL_GUARD(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength));
		std::string log(logLength, '\0');
		GL_GUARD(glGetShaderInfoLog(shader, logLength, nullptr, log.data()));
		fprintf(stderr, "Failed to build shader - Shader log : %s\n", log.data());
		return 0;
	} else {
		return shader;
	}
}

GLuint loadShader(const char* path, GLenum type) {

	std::string buff;
	if (std::ifstream file{ path, std::ios::binary | std::ios::ate }) {
		auto size = file.tellg();
		std::string str(size, '\0');
		file.seekg(0);
		if (file.read(&str[0], size))
			buff = str;
		file.close();
	} else {
		fprintf(stderr, "failed to open file %s\n", path);
		return 0;
	}

	return createShader(buff, type);
}

GLuint createRenderPipeline(GLuint vertexShader, GLuint fragmentShader) {

	auto program = GL_GUARD(glCreateProgram());
	GL_GUARD(glAttachShader(program, vertexShader));
	GL_GUARD(glAttachShader(program, fragmentShader));
	GL_GUARD(glLinkProgram(program));
	GLint linked;
	GL_GUARD(glGetProgramiv(program, GL_LINK_STATUS, &linked));
	GL_GUARD(glDetachShader(program, vertexShader));
	GL_GUARD(glDetachShader(program, fragmentShader));
	GL_GUARD(glDeleteShader(vertexShader));
	GL_GUARD(glDeleteShader(fragmentShader));

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

void bindTexture(GPUBinding& binding) { GL_GUARD(glBindTextureUnit(binding.target, binding.id)); }
void bindUBO(GPUBinding& binding) { GL_GUARD(glBindBufferRange(GL_UNIFORM_BUFFER, binding.target, binding.id, 0, binding.size)); }
void bindSSBO(GPUBinding& binding) { GL_GUARD(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding.target, binding.id, 0, binding.size)); }
		// GL_GUARD(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo.binding, ssbo.id));
void unbindTexture(GPUBinding& binding) { GL_GUARD(glBindTextureUnit(binding.target, 0)); }
void unbindUBO(GPUBinding& binding) { GL_GUARD(glBindBufferRange(GL_UNIFORM_BUFFER, binding.target, 0, 0, binding.size)); }
void unbindSSBO(GPUBinding& binding) { GL_GUARD(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding.target, 0, 0, binding.size)); }
		// GL_GUARD(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo.binding, ssbo.id));

GPUBinding	bind(GLuint id, GLuint target, size_t size) { return GPUBinding { id, target, (GLsizeiptr)size }; }
GPUBinding	bind(const Textures::Texture& mapping, GLuint target) { return GPUBinding { mapping.id, target, mapping.dimensions.x * mapping.dimensions.y * mapping.channels }; }
template<typename T> GPUBinding	bind(const MappedObject<T>& mapping, GLuint target) { return GPUBinding { mapping.id, target, sizeof(mapping.obj) }; }
template<typename T> GPUBinding	bind(const MappedBuffer<T>& mapping, GLuint target) { return GPUBinding { mapping.id, target, mapping.obj.size_bytes() }; }

constexpr std::span<GPUBinding> noBinds = {};

void draw(
	GLuint pipeline,
	GLuint vao,
	GLsizei indexCount,
	int instanceCount = 1,
	std::span<GPUBinding> textures = noBinds,
	std::span<GPUBinding> ssbos = noBinds,
	std::span<GPUBinding> ubos = noBinds
) {
	GL_GUARD(glUseProgram(pipeline));
	for (auto &&texture : textures) bindTexture(texture);
	for (auto &&ubo : ubos) bindUBO(ubo);
	for (auto &&ssbo : ssbos) bindSSBO(ssbo);
	GL_GUARD(glBindVertexArray(vao));

	GL_GUARD(glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr, instanceCount));

	GL_GUARD(glBindVertexArray(0));
	for (auto &&ssbo : ssbos) unbindSSBO(ssbo);
	for (auto &&ubo : ubos) unbindUBO(ubo);
	for (auto &&texture : textures) unbindTexture(texture);
	GL_GUARD(glUseProgram(0));
}


#endif
