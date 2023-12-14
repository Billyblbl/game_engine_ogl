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
#include <transform.cpp>
#include <entity.cpp>

#include <spall/profiling.cpp>

//TODO remove fstream dependency

GLuint create_shader(string source, GLenum type) {
	auto shader = GL_GUARD(glCreateShader(type));

	const cstrp content[] = {
		"#version 450 core\n"
		"#define ", GLtoString(type).substr(3).data(),"\n\n", // shader type macro
		"#ifdef VERTEX_SHADER\n"
		"#define pass out\n"
		"#endif\n"
		"#ifdef FRAGMENT_SHADER\n"
		"#define pass in\n"
		"#endif\n",
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
		char log[log_length + 1];
		memset(log, 0, log_length + 1);
		GL_GUARD(glGetShaderInfoLog(shader, log_length, nullptr, log));
		fprintf(stderr, "Failed to build %s shader - Shader log : %s\n", GLtoString(type).data(), log);
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
		return create_shader(string(buffer, size), type);
	} else {
		return (fprintf(stderr, "failed to open file %s\n", path), 0);
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
		// case GPUBinding::Texture: printf("Binding texture %u to %u\n", binding.id, binding.target); GL_GUARD(glBindTextureUnit(binding.target, binding.id)); break;
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


void draw(
	GLuint pipeline,
	const RenderMesh& mesh,
	u32 instance_count = 1,
	Array<const GPUBinding> bindings = {}
) {
	GL_GUARD(glUseProgram(pipeline));
	GL_GUARD(glBindVertexArray(mesh.vao));
	for (auto&& binding : bindings) bind(binding);
	// TODO redo render mesh & draw usage
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

struct Pipeline {
	GLuint id;

	void operator()(
		const RenderMesh& mesh,
		u32 instance_count = 1,
		Array<const GPUBinding> bindings = {}
		) const {
		draw(id, mesh, instance_count, bindings);
	}

	void operator()(
		const RenderMesh& mesh,
		u32 instance_count,
		LiteralArray<GPUBinding> bindings
		) const {
		draw(id, mesh, instance_count, larray(bindings));
	}
};

Pipeline create_render_pipeline(GLuint vertex_shader, GLuint fragment_shader) {
	if (vertex_shader == 0 || fragment_shader == 0) {
		fprintf(stderr, "Failed to build render pipeline, invalid shader\n");
		return { 0 };
	}

	auto program = GL_GUARD(glCreateProgram());
	GL_GUARD(glAttachShader(program, vertex_shader));
	GL_GUARD(glAttachShader(program, fragment_shader));
	GL_GUARD(glLinkProgram(program));
	GLint linked;
	GL_GUARD(glGetProgramiv(program, GL_LINK_STATUS, &linked));
	GL_GUARD(glDetachShader(program, vertex_shader));
	GL_GUARD(glDetachShader(program, fragment_shader));
	if (!linked) {
		GLint logLength;
		GL_GUARD(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength));
		char log[logLength + 1];
		log[logLength] = '\0';
		GL_GUARD(glGetProgramInfoLog(program, logLength, nullptr, log));
		fprintf(stderr, "Failed to build render pipeline %u, pipeline program log : %s\n", program, log);
		return { 0 };
	} else
		return { program };
}

Pipeline load_pipeline(const char* path) {
	printf("Loading pipeline %s\n", path);
	fflush(stdout);
	if (std::ifstream file{ path, std::ios::binary | std::ios::ate }) {
		usize size = file.tellg();
		char buffer[size + 1] = { 0 };
		file.seekg(0);
		file.read(buffer, size);
		file.close();
		auto vert = create_shader(string(buffer, size), GL_VERTEX_SHADER);
		auto frag = create_shader(string(buffer, size), GL_FRAGMENT_SHADER);
		auto pipeline = create_render_pipeline(vert, frag);
		GL_GUARD(glDeleteShader(vert));
		GL_GUARD(glDeleteShader(frag));
		return pipeline;
	} else {
		fprintf(stderr, "failed to open file %s\n", path);
		return { 0 };
	}

}

void destroy_pipeline(Pipeline& pipeline) {
	GL_GUARD(glDeleteProgram(pipeline.id));
	pipeline.id = 0;
}

inline GPUBinding bind_to(const TexBuffer& mapping, GLuint target) { return GPUBinding{ GPUBinding::Texture, mapping.id, target, mapping.dimensions.x * mapping.dimensions.y * mapping.dimensions.z * mapping.dimensions.w }; }
template<typename T> inline GPUBinding bind_to(const MappedObject<T>& mapping, GLuint target) { return GPUBinding{ GPUBinding::UBO, mapping.id, target, sizeof(T) }; }
template<typename T> inline GPUBinding bind_to(const MappedBuffer<T>& mapping, GLuint target) { return GPUBinding{ GPUBinding::SSBO, mapping.id, target, (GLsizeiptr)mapping.content.size_bytes() }; }

void wait_gpu() {
	GL_GUARD(glFinish());
}

#include <sprite.cpp>
#include <system_editor.cpp>
#include <atlas.cpp>

struct RenderTarget {
	FrameBuffer fbf = default_framebuffer;
	v4f32 clear_color = v4f32(v3f32(0.3), 1);
};

bool EditorWidget(const cstr label, RenderTarget& target) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer { ImGui::TreePop(); };
		changed |= EditorWidget(label, target.fbf);
		changed |= ImGui::ColorPicker4("Clear Color", glm::value_ptr(target.clear_color));
	}
	return changed;
}

struct Camera {
	Spacial2D* pov = null;
	OrthoCamera* projection = null;
	RenderTarget* target = null;
	operator m4x4f32() const { return m4x4f32(*projection) * inverse(m4x4f32(pov->transform)); }
};

void render(const Camera& camera, auto commands) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	using namespace glm;
	begin_render(camera.target->fbf);
	clear(camera.target->fbf, camera.target->clear_color);
	commands(camera);
}

#endif
