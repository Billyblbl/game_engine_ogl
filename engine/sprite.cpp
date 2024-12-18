#ifndef GSPRITE
# define GSPRITE

#include <rendering.cpp>
#include <textures.cpp>
#include <imgui_extension.cpp>
#include <image.cpp>

struct Sprite {
	rtu32 view;
	v2f32 dimensions;
	f32 depth;
};

bool EditorWidget(const cstr label, Sprite& data) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("View", data.view);
		changed |= EditorWidget("Dimensions", data.dimensions);
		changed |= EditorWidget("Depth", data.depth);
		ImGui::TreePop();
	}
	return changed;
}

template<typename T, i32 D> inline glm::vec<D, T> rect_to_world(reg_polytope<glm::vec<D, T>> rect, glm::vec<D, T> v) {
	return v * (rect.max - rect.min) + rect.min;
}

template<typename T, i32 D> inline reg_polytope<glm::vec<D, T>> rect_in_rect(reg_polytope<glm::vec<D, T>> reference, reg_polytope<glm::vec<D, T>> rect) {
	return { rect_to_world(reference, rect.min), rect_to_world(reference, rect.max) };
}

inline rtu32 sub_rect(rtu32 source, rtu32 sub) { return { sub.min + source.min, sub.max + source.min }; }

struct SpriteRenderer {
	struct Scene {
		m4x4f32 view_projection;
		v4u32 atlas_dimensions;
		f32 alpha_discard;
	};

	struct Instance {
		m4x4f32 matrix;
		rtu32 uv_rect;
		v4f32 dimensions; //x, y -> rect dims, z -> rect depths, w -> padding
	};

	GLuint pipeline;
	GPUGeometry rect;

	struct {
		ShaderInput atlas;
		ShaderInput scene;
		ShaderInput instances;
	} inputs;

	static Instance make_instance(const Sprite& sprite, const m4x4f32& matrix) {
		Instance instance;
		instance.uv_rect = sprite.view;
		instance.matrix = matrix * glm::scale(v3f32(sprite.dimensions, 1));
		instance.dimensions = v4f32(sprite.dimensions, sprite.depth, 0);
		return instance;
	}

	void operator()(
		Array<Instance> sprites,
		const m4x4f32& vp,
		const TexBuffer& textures
		) {
		GL_GUARD(glUseProgram(pipeline)); defer{ GL_GUARD(glUseProgram(0)); };
		GL_GUARD(glBindVertexArray(rect.vao.id)); defer{ GL_GUARD(glBindVertexArray(0)); };
		inputs.atlas.bind_texture(textures.id); defer{ inputs.atlas.unbind(); };
		inputs.instances.bind_content(sprites); defer{ inputs.instances.unbind(); };
		inputs.scene.bind_object(Scene{ vp, textures.dimensions, 0.01f }); defer{ inputs.scene.unbind(); };
		GL_GUARD(glDrawElementsInstanced(rect.vao.draw_mode, rect.element_count, rect.vao.index_type, null, sprites.size()));
	}

	static SpriteRenderer load(const cstr pipeline_path = "./shaders/sprite.glsl", u32 max_draw_batch = 256, const GPUGeometry* mesh = null) {
		PROFILE_SCOPE(__PRETTY_FUNCTION__);
		SpriteRenderer rd;
		rd.pipeline = load_pipeline(pipeline_path);
		rd.rect = mesh ? *mesh : create_rect_mesh(v2f32(1));
		rd.inputs.atlas = ShaderInput::create_slot(rd.pipeline, ShaderInput::Texture, "atlas");
		rd.inputs.instances = ShaderInput::create_slot(rd.pipeline, ShaderInput::SSBO, "Entities", sizeof(Instance) * max_draw_batch);
		rd.inputs.scene = ShaderInput::create_slot(rd.pipeline, ShaderInput::UBO, "Scene", sizeof(Scene));
		return rd;
	}

	void release() {
		inputs.instances.release();
		inputs.scene.release();
		inputs.atlas.release();
		rect.release();
		destroy_pipeline(pipeline);
	}
};

#endif
