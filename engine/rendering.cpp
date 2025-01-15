#ifndef GRENDERING
# define GRENDERING

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
#include <pipeline.cpp>

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

struct TextureBinding {
	Array<GLuint> textures;
	GLuint target;
};

struct BufferObjectBinding {
	GLuint buffer;
	GLenum type;
	num_range<u32> range;
	GLuint target;
};

struct VertexBinding {
	GLuint buffer;
	Array<GLuint> targets;
	GLintptr offset;
	GLsizeiptr stride;
	GLuint divisor;
};

struct ClearCommand {
	GLbitfield attachements = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	v4f32 color = v4f32(v3f32(0.3), 1);
	f32 depth = 1;
	GLint stencil = 0;
};

void clear(const ClearCommand& cmd) {
	if (cmd.attachements & GL_COLOR_BUFFER_BIT)
		GL_GUARD(glClearColor(cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a));
	if (cmd.attachements & GL_DEPTH_BUFFER_BIT)
		GL_GUARD(glClearDepthf(cmd.depth));
	if (cmd.attachements & GL_STENCIL_BUFFER_BIT)
		GL_GUARD(glClearStencil(cmd.stencil));
	GL_GUARD(glClear(cmd.attachements));
}

struct RenderCommand {
	GLuint pipeline;
	enum DrawType : u32 {
		D_CLEAR = 0,	//* glClear
		D_MDEI,				//* glMultiDrawElementsIndirect
		D_MDAI,				//* glMultiDrawArraysIndirect
		D_DE,					//* glDrawElementsInstancedBaseVertexBaseInstance
		D_DA,					//* glDrawArraysInstanced
		D_MDE,				//* glMultiDrawElementsBaseVertex
		D_MDA,				//* glMultiDrawArrays
		D_DRAWTYPE_COUNT
	} draw_type;
	union {
		struct {
			GLuint buffer;
			GLsizei stride;
			num_range<GLsizei> range;
		} d_indirect;
		ClearCommand d_clear;
		DrawCommandElement d_element;
		DrawCommandVertex d_vertex;
		Array<DrawCommandElement> d_melements;
		Array<DrawCommandVertex> d_mvertices;
	} draw;
	GLuint vao;
	struct {
		GLuint buffer;
		GLenum index_type;
		GLenum primitive;
	} ibo;
	Array<VertexBinding> vertex_buffers;
	Array<TextureBinding> textures;
	Array<BufferObjectBinding> buffers;
};

void render_cmd(const RenderCommand& batch) {
	if (batch.draw_type == RenderCommand::D_CLEAR)
		return clear(batch.draw.d_clear);
	assert((batch.draw_type < RenderCommand::D_DRAWTYPE_COUNT) && "Unsupported draw type");
	glUseProgram(batch.pipeline); defer { glUseProgram(0); };
	struct { GLuint next[R_TYPE_COUNT]; } bindings = { .next = {0, 0, 0, 0, 0, 0} };

	//* configure & bind vertex buffers, index buffers, vertex array
	for (auto vbo : batch.vertex_buffers) {
		glVertexArrayVertexBuffer(batch.vao, bindings.next[R_VERT], vbo.buffer, vbo.offset, vbo.stride);
		glVertexArrayBindingDivisor(batch.vao, bindings.next[R_VERT], vbo.divisor);
		for (auto target : vbo.targets)
			glVertexArrayAttribBinding(batch.vao, target, bindings.next[R_VERT]);
		bindings.next[R_VERT]++;
	}
	glVertexArrayElementBuffer(batch.vao, batch.ibo.buffer);
	glBindVertexArray(batch.vao); defer { glBindVertexArray(0); };

	//* push texture uniforms
	const static u32 max_tex = get_max_textures_combined();
	GLint tex_units[max_tex];
	for (auto tex : batch.textures) {
		if (tex.textures.size() == 0)
			continue;
		//* bind textures to units
		auto tex_list = List { .capacity = carray(tex_units, max_tex), .current = 0 };
		for (auto id : tex.textures) {
			assert(max_tex >= bindings.next[R_TEX] && "Not enough texture units");
			GL_GUARD(glBindTextureUnit(bindings.next[R_TEX], id));
			tex_list.push(bindings.next[R_TEX]++);
		}
		//* write bound units to uniform
		if (tex_list.current == 1)
			GL_GUARD(glProgramUniform1i(batch.pipeline, tex.target, tex_list.used()[0]));
		else
			GL_GUARD(glProgramUniform1iv(batch.pipeline, tex.target, tex_list.current, tex_list.used().data()));
	};
	//* defered unbinds
	defer { for (GLuint unit = 0; unit < bindings.next[R_TEX]; unit++) glBindTextureUnit(unit, 0); };

	//* bind buffer objects
	for (auto buf : batch.buffers) {
		u32 rindex = type_to_rindex(buf.type);
		assert(rindex >= R_SSBO && rindex <= R_TBO && "Invalid buffer type");
		if (buf.range.size() == 0)
			GL_GUARD(glBindBufferBase(buf.type, bindings.next[rindex], buf.buffer));
		else
			GL_GUARD(glBindBufferRange(buf.type, bindings.next[rindex], buf.buffer, buf.range.min, buf.range.size()));
		auto binding = bindings.next[rindex]++;
		switch (buf.type) {
			case GL_UNIFORM_BUFFER: glUniformBlockBinding(batch.pipeline, buf.target, binding); break;
			case GL_SHADER_STORAGE_BUFFER: glShaderStorageBlockBinding(batch.pipeline, buf.target, binding); break;
			default: assert(0 && "Invalid buffer type");
		}
	}
	defer { //* defered unbinds
		for (i32 buf_type : i32xrange { R_SSBO, R_TBO }) for (GLuint binding : idx_range<GLuint>{ 0, bindings.next[buf_type] })
			glBindBufferBase(rindex_to_type[buf_type], binding, 0);
	};

	//* dispatch
	switch(batch.draw_type) {
		case RenderCommand::D_MDEI: {
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batch.draw.d_indirect.buffer); defer { glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0); };
			return glMultiDrawElementsIndirect(
				batch.ibo.primitive,
				batch.ibo.index_type,
				(void*)u64(batch.draw.d_indirect.range.min * batch.draw.d_indirect.stride),
				batch.draw.d_indirect.range.size(),
				batch.draw.d_indirect.stride
			);
		}
		case RenderCommand::D_MDAI:{
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batch.draw.d_indirect.buffer); defer { glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0); };
			return glMultiDrawArraysIndirect(
				batch.ibo.primitive,
				(void*)u64(batch.draw.d_indirect.range.min * batch.draw.d_indirect.stride),
				batch.draw.d_indirect.range.size(),
				batch.draw.d_indirect.stride
			);
		}
		case RenderCommand::D_DE: return glDrawElementsInstancedBaseVertexBaseInstance(
			batch.ibo.primitive,
			batch.draw.d_element.count,
			batch.ibo.index_type,
			(void*)(batch.draw.d_element.first_index * index_size(batch.ibo.index_type)),
			batch.draw.d_element.instance_count,
			batch.draw.d_element.base_vertex,
			batch.draw.d_element.base_instance
		);
		case RenderCommand::D_DA: return glDrawArraysInstancedBaseInstance(
			batch.ibo.primitive,
			batch.draw.d_vertex.first_vertex,
			batch.draw.d_vertex.count,
			batch.draw.d_vertex.instance_count,
			batch.draw.d_vertex.base_instance
		);
		case RenderCommand::D_MDE: {
			GLsizei counts[batch.draw.d_melements.size()];
			GLint base_vertices[batch.draw.d_melements.size()];
			void* indices[batch.draw.d_melements.size()];
			for (u32 i = 0; i < batch.draw.d_melements.size(); i++) {
				counts[i] = batch.draw.d_melements[i].count;
				base_vertices[i] = batch.draw.d_melements[i].base_vertex;
				indices[i] = (void*)(batch.draw.d_melements[i].first_index * index_size(batch.ibo.index_type));
			}
			return glMultiDrawElementsBaseVertex(
				batch.ibo.primitive,
				counts,
				batch.ibo.index_type,
				indices,
				batch.draw.d_melements.size(),
				base_vertices
			);
		}
		case RenderCommand::D_MDA: {
			GLint firsts[batch.draw.d_mvertices.size()];
			GLsizei counts[batch.draw.d_mvertices.size()];
			for (u32 i = 0; i < batch.draw.d_mvertices.size(); i++) {
				firsts[i] = batch.draw.d_mvertices[i].first_vertex;
				counts[i] = batch.draw.d_mvertices[i].count;
			}
			return glMultiDrawArrays(
				batch.ibo.primitive,
				firsts,
				counts,
				batch.draw.d_mvertices.size()
			);
		}
		default: return assert(0 && "Unsupported draw type");
	}
}

bool EditorWidget(const cstr label, ClearCommand& cmd) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer { ImGui::TreePop(); };
		changed |= ImGui::mask_flags("attachements", cmd.attachements, {
			NamedEnum(GLbitfield, GL_COLOR_BUFFER_BIT),
			NamedEnum(GLbitfield, GL_DEPTH_BUFFER_BIT),
			NamedEnum(GLbitfield, GL_STENCIL_BUFFER_BIT)
		});
		changed |= ImGui::ColorEdit4("color", &cmd.color.x);
		changed |= EditorWidget("depth", cmd.depth);
		changed |= EditorWidget("stencil", cmd.stencil);
	}
	return changed;
}


#endif
