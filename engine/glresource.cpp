#ifndef GGLRESOURCE
# define GGLRESOURCE

#include <glutils.cpp>

struct GLScope {
	Arena arena;
	List<GLuint> buffers;
	List<GLuint> textures;
	List<GLuint> framebuffers;
	List<GLuint> pipelines;
	List<GLuint> samplers;
	List<GLuint> vaos;

	static GLScope create(
		Arena&& arena,
		u64 buffer_count = 10,
		u64 texture_count = 5,
		u64 framebuffer_count = 1,
		u64 pipeline_count = 5,
		u64 sampler_count = 0,
		u64 vaos_count = 5
	) {
		return {
			.arena = arena,
			.buffers = List { .capacity = arena.push_array<GLuint>(buffer_count), .current = 0 },
			.textures = List { .capacity = arena.push_array<GLuint>(texture_count), .current = 0 },
			.framebuffers = List { .capacity = arena.push_array<GLuint>(framebuffer_count), .current = 0 },
			.pipelines = List { .capacity = arena.push_array<GLuint>(pipeline_count), .current = 0 },
			.samplers = List { .capacity = arena.push_array<GLuint>(sampler_count), .current = 0 },
			.vaos = List { .capacity = arena.push_array<GLuint>(vaos_count), .current = 0 }
		};
	}

	template<List<GLuint> GLScope::* type> auto push(GLuint resource) { return (this->*type).push_growing(arena, resource); }

	void release() {
		for (auto&& pipeline : pipelines.used())
			glDeleteProgram(pipeline);
		glDeleteSamplers(samplers.current, samplers.capacity.data());
		glDeleteTextures(textures.current, textures.capacity.data());
		glDeleteFramebuffers(framebuffers.current, framebuffers.capacity.data());
		glDeleteVertexArrays(vaos.current, vaos.capacity.data());
		glDeleteBuffers(buffers.current, buffers.capacity.data());
	}

	static GLScope& global() {
		static GLScope scope = create(Arena::from_vmem(1 << 16, Arena::DEFAULT_VMEM_FLAGS | Arena::ALLOW_MOVE_MORPH));
		return scope;
	}
};

#endif
