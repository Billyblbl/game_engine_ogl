#ifndef GRENDERING
# define GRENDERING

#include <GL/glew.h>
#include <GL/glext.h>
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

i32 get_max_textures_frag() {
	static i32 max_textures = 0;
	if (max_textures == 0) {
		GL_GUARD(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_textures));
	}
	assert(max_textures >= 16);
	return max_textures;
}

GLuint create_shader(string source, GLenum type) {
	assert(*source.end() == '\0' && "Shader source must be null terminated");
	auto shader = GL_GUARD(glCreateShader(type));
	char max_texture_text[16];
	sprintf(max_texture_text, "%d", get_max_textures_frag());
	const cstrp content[] = {
		"#version 460 core\n"//0
		"#define ", GLtoString(type).substr(3).data(),"\n\n", //* shader type | line 1,2
		"#ifdef VERTEX_SHADER\n"//3
		"#define pass out\n"//4
		"#endif\n"//5
		"#ifdef FRAGMENT_SHADER\n"//6
		"#define pass in\n"//7
		"#endif\n",//8
		"#define MAX_TEXTURE_IMAGE_UNITS ", max_texture_text, "\n",//9
		source.data(), "\n"//n
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
		fprintf(stderr, "Failed to build %s shader - Shader log : \n%s\n", GLtoString(type).data(), log);
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
		char buffer[size + 1];
		memset(buffer, 0, size + 1);
		file.seekg(0);
		file.read(buffer, size);
		file.close();
		return create_shader(string(buffer, size), type);
	} else {
		return (fprintf(stderr, "failed to open file %s\n", path), 0);
	}
}

GLint init_binding_texture(GLuint pipeline, const cstr name) {
	return GL_GUARD(glGetUniformLocation(pipeline, name));
}

GLint init_binding_ubo(GLuint pipeline, const cstr name, GLint binding) {
	auto index = GL_GUARD(glGetProgramResourceIndex(pipeline, GL_UNIFORM_BLOCK, name));
	GL_GUARD(glUniformBlockBinding(pipeline, index, binding));
	return binding;
}

GLint init_binding_ssbo(GLuint pipeline, const cstr name, GLint binding) {
	auto index = GL_GUARD(glGetProgramResourceIndex(pipeline, GL_SHADER_STORAGE_BLOCK, name));
	GL_GUARD(glShaderStorageBlockBinding(pipeline, index, binding));
	return binding;
}

GLuint push_texture(GLint location, GLuint unit, TexBuffer texture) {
	texture.bind_unit(unit);
	GL_GUARD(glUniform1i(location, unit));
	return unit + 1;
}

GLuint push_texture_array(GLint location, GLuint unit, Array<TexBuffer*> textures) {
	GLint units_buffer[textures.size()];
	auto units = List { carray(units_buffer, textures.size()), 0 };
	for (auto& t : textures)
		t->bind_unit(units.push(GLint(unit++)));
	GL_GUARD(glUniform1iv(location, textures.size(), units_buffer));
	return unit;
}

struct BufferBinding {
	enum BufferTarget : GLenum {
		None = 0,
		UBO = GL_UNIFORM_BUFFER,
		SSBO = GL_SHADER_STORAGE_BUFFER,
		ACO = GL_ATOMIC_COUNTER_BUFFER,
		TBO = GL_TRANSFORM_FEEDBACK_BUFFER
	} target;
	GLint index;
	GPUBuffer buffer;
	static BufferBinding create(BufferTarget target, GLint index, u64 size, GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT| GL_DYNAMIC_STORAGE_BIT) {
		return {
			.target = target,
			.index = index,
			.buffer = GPUBuffer::create(size, flags)
		 };
	}

	void bind(num_range<u64> range = {}) const { buffer.bind(target, index, range); }
	void unbind() const { buffer.unbind(target, index); }

	template<typename T> void push_array(Array<const T> data) { bind(buffer.write(cast<byte>(data))); }
	template<typename T> void push(const T& data) { push_array(carray(&data, 1)); }

	struct Sequencer {
		GLuint pipeline;
		GLint next_ssbo = 0;
		GLint next_ubo = 0;
		GLint next_aco = 0;
		GLint next_tbo = 0;

		BufferBinding next(BufferTarget target, const cstr name, u64 size, GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT| GL_DYNAMIC_STORAGE_BIT) {
			switch (target) {
				case UBO: return BufferBinding::create(target, init_binding_ubo(pipeline, name, next_ubo++), size, flags);
				case SSBO: return BufferBinding::create(target, init_binding_ssbo(pipeline, name, next_ssbo++), size, flags);
				case ACO: return BufferBinding::create(target, next_aco++, size, flags);//TODO implement binding if applicable
				case TBO: return BufferBinding::create(target, next_tbo++, size, flags);//TODO implement binding if applicable
				case None: { panic(); }
			}
			return {};
		}
	};

};

//TODO remove deprecated
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
		case None: { panic(); } break;
		}
		return range;
	}

	void unbind() const {
		switch (type) {
		case Texture: { GL_GUARD(glBindTextureUnit(id, 0)); } break;
		case UBO: { GL_GUARD(glBindBufferRange(GL_UNIFORM_BUFFER, id, 0, 0, 0)); } break;
		case SSBO: { GL_GUARD(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, id, 0, 0, 0)); } break;
		case None: { panic(); } break;
		}
	}

	static ShaderInput create_slot(GLuint pipeline, Type type, const cstr name, u64 backing_size = 0) {
		ShaderInput slot;
		slot.name = name;
		slot.type = type;
		switch (type) {
		case Texture: {
			auto loc = GL_GUARD(glGetUniformLocation(pipeline, name));
			GLuint unit = 0;
			GL_GUARD(glGetnUniformuiv(pipeline, loc, sizeof(GLuint), &unit));
			slot.id = unit;
		} break;
		case UBO: case SSBO: {
			auto index = GL_GUARD(glGetProgramResourceIndex(pipeline, type, name));
			constexpr GLenum prop[] = { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE };
			constexpr auto prop_count = array_size(prop);
			GLint value[prop_count];
			GL_GUARD(glGetProgramResourceiv(pipeline, type, index, prop_count, prop, prop_count, NULL, value));
			auto buffer_binding = value[0];
			auto buffer_size = value[1];
			slot.id = buffer_binding;
			slot.backing_buffer = GPUBuffer::create(max(u64(buffer_size), backing_size), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT);
			break;
		}
		case None: { panic(); } break;
		}
		return slot;
	}

	void release() {
		switch (type) {
		case UBO:
		case SSBO: { backing_buffer.release(); }
		default: break;
		}
	}

	void bind_texture(GLuint tid) { texture = tid; bind(); }
	template<typename T> num_range<u64> bind_content(Array<T> content, u64 offset = 0) {
		if (content.size() == 0)
			return {};
		if (backing_buffer.size < content.size_bytes() + offset)//TODO old content conservation
			backing_buffer.resize(content.size_bytes() + offset);
		return bind(backing_buffer.write(cast<byte>(content), offset));
	}
	template<typename T> num_range<u64> bind_object(const T& content, u64 offset = 0) { return bind_content(carray(&content, 1), offset); }
};

//*
//* https://www.khronos.org/opengl/wiki/Vertex_Rendering#Multi-Draw
// TODO glMultiDrawElementsIndirect
//*

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
	if (!linked) {
		GLint logLength;
		GL_GUARD(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength));
		char log[logLength + 1];
		log[logLength] = '\0';
		GL_GUARD(glGetProgramInfoLog(program, logLength, nullptr, log));
		fprintf(stderr, "Failed to build render pipeline %u, pipeline program log : %s\n", program, log);
		return 0;
	} else {
		return program;
	}
}

struct DrawCommandElement {
	GLuint count;
	GLuint instance_count;
	GLuint first_index;
	GLint base_vertex;
	GLuint base_instance;
};

struct DrawCommandVertex {
	GLuint count;
	GLuint instance_count;
	GLuint first_vertex;
	GLuint base_instance;
};

void describe(GLuint program) {
	struct {
		GLenum id;
		string name;
		u32 prop_count;
		GLenum properties[10];
		string property_names[10];
	} interfaces[] = {
		{GL_UNIFORM, "GL_UNIFORM", 6, { GL_TYPE, GL_ARRAY_SIZE, GL_OFFSET, GL_BLOCK_INDEX, GL_ARRAY_STRIDE, GL_LOCATION }, { "GL_TYPE", "GL_ARRAY_SIZE", "GL_OFFSET", "GL_BLOCK_INDEX", "GL_ARRAY_STRIDE", "GL_LOCATION" }},
		{GL_UNIFORM_BLOCK, "GL_UNIFORM_BLOCK", 4, { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES, GL_ACTIVE_VARIABLES }, { "GL_BUFFER_BINDING", "GL_BUFFER_DATA_SIZE", "GL_NUM_ACTIVE_VARIABLES", "GL_ACTIVE_VARIABLES" }},
		{GL_PROGRAM_INPUT, "GL_PROGRAM_INPUT", 5, { GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION, GL_IS_PER_PATCH, GL_LOCATION_COMPONENT }, { "GL_TYPE", "GL_ARRAY_SIZE", "GL_LOCATION", "GL_IS_PER_PATCH", "GL_LOCATION_COMPONENT" }},
		{GL_PROGRAM_OUTPUT, "GL_PROGRAM_OUTPUT", 5, { GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION, GL_IS_PER_PATCH, GL_LOCATION_COMPONENT }, { "GL_TYPE", "GL_ARRAY_SIZE", "GL_LOCATION", "GL_IS_PER_PATCH", "GL_LOCATION_COMPONENT" }},
		{GL_BUFFER_VARIABLE, "GL_BUFFER_VARIABLE", 7, { GL_TYPE, GL_ARRAY_SIZE, GL_OFFSET, GL_BLOCK_INDEX, GL_ARRAY_STRIDE, GL_TOP_LEVEL_ARRAY_SIZE, GL_TOP_LEVEL_ARRAY_STRIDE }, { "GL_TYPE", "GL_ARRAY_SIZE", "GL_OFFSET", "GL_BLOCK_INDEX", "GL_ARRAY_STRIDE", "GL_TOP_LEVEL_ARRAY_SIZE", "GL_TOP_LEVEL_ARRAY_STRIDE" }},
		{GL_SHADER_STORAGE_BLOCK, "GL_SHADER_STORAGE_BLOCK", 4, {GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES, GL_ACTIVE_VARIABLES }, {"GL_BUFFER_BINDING", "GL_BUFFER_DATA_SIZE", "GL_NUM_ACTIVE_VARIABLES", "GL_ACTIVE_VARIABLES" }},
	};

	printf("Program %u :\n", program);
	for (auto& interface : interfaces) {
		printf("-- interface %s :\n", interface.name.data());
		GLint ressource_count = 0;
		GLint name_buffer_size = 0;
		GL_GUARD(glGetProgramInterfaceiv(program, interface.id, GL_ACTIVE_RESOURCES, &ressource_count));
		GL_GUARD(glGetProgramInterfaceiv(program, interface.id, GL_MAX_NAME_LENGTH, &name_buffer_size));
		GLint values[interface.prop_count];
		GLchar name[name_buffer_size];
		for (auto resource : u32xrange{ 0, ressource_count }) {
			GL_GUARD(glGetProgramResourceName(program, interface.id, resource, name_buffer_size, NULL, name));
			GL_GUARD(glGetProgramResourceiv(program, interface.id, resource, interface.prop_count, interface.properties, interface.prop_count, NULL, values));
			printf("Ressource %s : {\n", name);
			for (auto prop : u32xrange{ 0, interface.prop_count })
				printf("\t%s : %i\n", interface.property_names[prop].data(), values[prop]);
			printf("}\n");
		}
	}
	fflush(stdout);
}

GLuint load_pipeline(const char* path) {
	printf("Loading pipeline %s\n", path);
	fflush(stdout);
	//TODO discard all this fstream garbage
	if (std::ifstream file{ path, std::ios::binary | std::ios::ate }) {
		usize size = file.tellg();
		char buffer[size + 1];
		memset(buffer, 0, size + 1);
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
		return 0;
	}
}

void destroy_pipeline(GLuint& pipeline) {
	GL_GUARD(glDeleteProgram(pipeline));
	pipeline = 0;
}

void wait_gpu() {
	GL_GUARD(glFinish());
}

struct RenderTarget {
	FrameBuffer fbf = default_framebuffer;
	v4f32 clear_color = v4f32(v3f32(0.3), 1);
};

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

#include <system_editor.cpp>
bool EditorWidget(const cstr label, RenderTarget& target) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget(label, target.fbf);
		changed |= ImGui::ColorPicker4("Clear Color", glm::value_ptr(target.clear_color));
	}
	return changed;
}

#endif
