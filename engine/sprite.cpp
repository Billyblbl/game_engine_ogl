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

	Pipeline pipeline;
	MappedBuffer<Instance> instances_buffer;
	MappedObject<Scene> scene;
	RenderMesh rect;

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
		//! will silently fail to render the latter part of the sprites array if > instance_buffer size
		u32 content_size = min(sprites.size(), instances_buffer.content.size());
		memcpy(instances_buffer.content.data(), sprites.data(), content_size * sizeof(Instance));
		sync(instances_buffer);
		sync(scene, { vp, textures.dimensions, 0.01f });
		pipeline(rect, content_size,
			{
				bind_to(textures, 0),
				bind_to(instances_buffer, 0),
				bind_to(scene, 0),
			}
		);
	}

	static SpriteRenderer load(const cstr pipeline_path = "./shaders/sprite.glsl", GLsizeiptr max_draw_batch = 256, const RenderMesh* mesh = null) {
		SpriteRenderer rd;
		rd.pipeline = load_pipeline(pipeline_path);
		rd.instances_buffer = map_buffer<Instance>(max_draw_batch);
		rd.scene = map_object<SpriteRenderer::Scene>({});
		rd.rect = mesh ? *mesh : create_rect_mesh(v2f32(1));
		return rd;
	}

	void release() {
		destroy_pipeline(pipeline);
		unmap(instances_buffer);
		unmap(scene);
		delete_mesh(rect);
	}
};

#endif
