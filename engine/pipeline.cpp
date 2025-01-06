#ifndef GPIPELINE
# define GPIPELINE

#include <glutils.cpp>
#include <fstream>
#include <glresource.cpp>
//TODO remove fstream dependency

i32 get_max_textures_frag() {
	static i32 max_textures = 0;
	if (max_textures == 0) {
		GL_GUARD(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_textures));
	}
	assert(max_textures >= 16);
	return max_textures;
}

i32 get_max_textures_combined() {
	static i32 max_textures = 0;
	if (max_textures == 0) {
		GL_GUARD(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_textures));
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

enum Resource : i32 {
	R_TEX = 0,
	R_SSBO,
	R_UBO,
	R_ACO,
	R_TBO,
	R_VERT,
	R_TYPE_COUNT
};

i32 type_to_rindex(GLenum type) {
	switch (type) {
		case GL_TEXTURE: return R_TEX;
		case GL_SHADER_STORAGE_BUFFER: return R_SSBO;
		case GL_UNIFORM_BUFFER: return R_UBO;
		case GL_ATOMIC_COUNTER_BUFFER: return R_ACO;
		case GL_TRANSFORM_FEEDBACK_BUFFER: return R_TBO;
		case GL_VERTEX_BINDING_BUFFER: return R_VERT;
		default: assert(0 && "Invalid buffer type");
	}
	return -1;
}

static constexpr GLenum rindex_to_type[] = {
	GL_TEXTURE,
	GL_SHADER_STORAGE_BUFFER,
	GL_UNIFORM_BUFFER,
	GL_ATOMIC_COUNTER_BUFFER,
	GL_TRANSFORM_FEEDBACK_BUFFER,
	GL_VERTEX_BINDING_BUFFER
};

GLuint get_shader_input(GLuint pipeline, const cstr name, Resource type) {
	switch (type) {
		case R_UBO: return GL_GUARD(glGetProgramResourceIndex(pipeline, GL_UNIFORM_BLOCK, name));
		case R_SSBO: return GL_GUARD(glGetProgramResourceIndex(pipeline, GL_SHADER_STORAGE_BLOCK, name));
		case R_TEX: {
			auto loc = GL_GUARD(glGetUniformLocation(pipeline, name));
			assert(loc >= 0);
			return loc;
		}
		case R_VERT: {
			auto loc = GL_GUARD(glGetAttribLocation(pipeline, name));
			assert(loc >= 0);
			return loc;
		}
		//todo ACO, TBO ?
		default: assert(0 && "Invalid buffer type"); return 0;
	}
}


GLuint create_render_pipeline(GLScope& ctx, GLuint vertex_shader, GLuint fragment_shader) {
	if (vertex_shader == 0 || fragment_shader == 0) {
		fprintf(stderr, "Failed to build render pipeline, invalid shader\n");
		return 0;
	}

	auto program = GL_GUARD(glCreateProgram());
	ctx.push<&GLScope::pipelines>(program);
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

GLuint load_pipeline(GLScope& ctx,const char* path) {
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
		auto pipeline = create_render_pipeline(ctx, vert, frag);
		GL_GUARD(glDeleteShader(vert));
		GL_GUARD(glDeleteShader(frag));
		return pipeline;
	} else {
		fprintf(stderr, "failed to open file %s\n", path);
		return 0;
	}
}

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
		char name[name_buffer_size];
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


#endif
