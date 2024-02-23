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
		"#define ", GLtoString(type).substr(3).data(),"\n\n", //* shader type macro
		"#ifdef VERTEX_SHADER\n"
		"#define pass out\n"
		"#endif\n"
		"#ifdef FRAGMENT_SHADER\n"
		"#define pass in\n"
		"#endif\n",
		source.data()//! expects source to be null terminated
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

struct ShaderInput {
	enum Type : GLenum { None = 0, Texture = GL_UNIFORM, UBO = GL_UNIFORM_BLOCK, SSBO = GL_SHADER_STORAGE_BLOCK } type;
	string name;
	GLuint id;
	union {
		GPUBuffer backing_buffer;
		GLuint texture;
	};

	num_range<u64> bind(num_range<u64> range = {}) const {
		switch (type) {
		case Texture: { GL_GUARD(glBindTextureUnit(id, texture)); } break;
		case UBO: { GL_GUARD(glBindBufferRange(GL_UNIFORM_BUFFER, id, backing_buffer.id, range.min, range.size())); } break;
		case SSBO: { GL_GUARD(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, id, backing_buffer.id, range.min, range.size())); } break;
		}
		return range;
	}

	void unbind() const {
		switch (type) {
		case Texture: { GL_GUARD(glBindTextureUnit(id, 0)); } break;
		case UBO: { GL_GUARD(glBindBufferRange(GL_UNIFORM_BUFFER, id, 0, 0, 0)); } break;
		case SSBO: { GL_GUARD(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, id, 0, 0, 0)); } break;
		}
	}

	static ShaderInput create_slot(GLuint pipeline, Type type, const cstr name, u64 backing_size = 0) {
		ShaderInput slot;
		slot.name = name;
		slot.type = type;
		switch (type) {
		case Texture: { slot.id = GL_GUARD(glGetProgramResourceLocation(pipeline, type, name)); } break;
		case UBO: case SSBO: {
			slot.id = GL_GUARD(glGetProgramResourceIndex(pipeline, type, name));
			slot.backing_buffer = GPUBuffer::create(backing_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT);
			break;
		}
		}
		return slot;
	}

	void release() {
		switch (type) {
		case UBO:
		case SSBO: { backing_buffer.release(); }
		}
	}

	void bind_texture(GLuint id) { texture = id; bind(); }
	template<typename T> num_range<u64> bind_content(Array<T> content, u64 offset = 0) { return bind(backing_buffer.write(cast<byte>(content), offset)); }
	template<typename T> num_range<u64> bind_object(const T& content, u64 offset = 0) { return bind_content(carray(&content, 1), offset); }
};

//*
//* https://www.khronos.org/opengl/wiki/Vertex_Rendering#Multi-Draw
// TODO glMultiDrawElementsIndirect
//*

GLuint create_render_pipeline(GLuint vertex_shader, GLuint fragment_shader) {
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
	} else {
		//* Debug stuff
		// printf("Pipeline input description:\n");
		// GLint uniform_count = 0;
		// GL_GUARD(glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &uniform_count));
		// const GLenum properties[4] = { GL_BLOCK_INDEX, GL_TYPE, GL_NAME_LENGTH, GL_LOCATION };
		// for (auto unif : u32xrange{ 0, uniform_count }) {
		// 	GLint values[4];
		// 	GL_GUARD(glGetProgramResourceiv(program, GL_UNIFORM, unif, 4, properties, 4, NULL, values));
		// 	char name[values[2]];
		// 	GL_GUARD(glGetProgramResourceName(program, GL_UNIFORM, unif, values[2], NULL, name));
		// 	printf("%s [%i] : %i -> %s, loc %i\n", name, values[0], values[1], GLtoString(values[1]).data(), values[3]);
		// }
		return { program };
	}
}

GLuint load_pipeline(const char* path) {
	printf("Loading pipeline %s\n", path);
	fflush(stdout);
	//TODO discard all this fstream garbage
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

void destroy_pipeline(GLuint& pipeline) {
	GL_GUARD(glDeleteProgram(pipeline));
	pipeline = 0;
}

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
		defer{ ImGui::TreePop(); };
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

inline void render(const Camera& camera, auto commands) {
	PROFILE_SCOPE(__PRETTY_FUNCTION__);
	using namespace glm;
	begin_render(camera.target->fbf);
	clear(camera.target->fbf, camera.target->clear_color);
	commands(camera);
}

#endif
