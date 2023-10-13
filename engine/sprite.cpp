#ifndef GSPRITE
# define GSPRITE

#include <rendering.cpp>
#include <textures.cpp>
#include <imgui_extension.cpp>
#include <image.cpp>

struct SpriteCursor {
	rtu32 view;
	u32 atlas_index;
};

struct Sprite {
	SpriteCursor cursor;
	v2f32 dimensions;
	f32 depth;
};

bool EditorWidget(const cstr label, SpriteCursor& data) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("View", data.view);
		changed |= EditorWidget("Atlas index", data.atlas_index);
		ImGui::TreePop();
	}
	return changed;
}

bool EditorWidget(const cstr label, Sprite& data) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("Cursor", data.cursor);
		changed |= EditorWidget("Dimensions", data.dimensions);
		changed |= EditorWidget("Depth", data.depth);
		ImGui::TreePop();
	}
	return changed;
}

constexpr SpriteCursor null_sprite = { { v2u32(0), v2u32(0) }, 0 };
constexpr SpriteCursor full_page(v2u32 size, u32 index) { return { { v2u32(0), size }, index }; }

TexBuffer load_texture(const cstr path, GPUFormat target_format = RGBA32F, TexType type = TX2D) {
	auto [format, dimensions, data, _] = load_image(path); defer{ stbi_image_free(data.data()); };
	if (dimensions.x * dimensions.y == 0)
		return fail_ret("can't create texture without image", null_tex);
	return create_texture(data, format, type, v4u32(dimensions, 1, 1), target_format);
}

inline rtf32 ratio(rtu32 area, v2u32 dimensions) { return { v2f32(area.min) / v2f32(dimensions), v2f32(area.max) / v2f32(dimensions) }; }

template<typename T, i32 D> inline glm::vec<D, T> rect_to_world(reg_polytope<glm::vec<D, T>> rect, glm::vec<D, T> v) {
	return v * (rect.max - rect.min) + rect.min;
}

template<typename T, i32 D> inline reg_polytope<glm::vec<D, T>> rect_in_rect(reg_polytope<glm::vec<D, T>> reference, reg_polytope<glm::vec<D, T>> rect) {
	return { rect_to_world(reference, rect.min), rect_to_world(reference, rect.max) };
}

inline rtu32 sub_rect(rtu32 source, rtu32 sub) { return { sub.min + source.min, sub.max + source.min }; }
inline SpriteCursor sub_sprite(SpriteCursor source, rtu32 sub) { return { sub_rect(source.view, sub), source.atlas_index }; }

SpriteCursor load_into(const Image& img, TexBuffer& texture, v2u32 upper_left = v2u32(0), u32 page = 0) {
	auto rect = rtu32{ upper_left, upper_left + img.dimensions };
	if (img.data.size() > 0 &&
		expect(texture.dimensions.x >= img.dimensions.x && texture.dimensions.y >= img.dimensions.y) &&//TODO proper erro handling
		upload_texture_data(texture, cast<byte>(img.data), img.format, slice_to_area<2>(rect, page)))
		return SpriteCursor{ rect, page };
	else
		return {};
}

SpriteCursor load_into(const cstr path, TexBuffer& texture, v2u32 upper_left = v2u32(0), u32 page = 0) {
	auto img = load_image(path); defer{ stbi_image_free(img.data.data()); };
	return load_into(img, texture, upper_left, page);
}

struct SpriteInstance {
	m4x4f32 matrix;
	rtu32 uv_rect;
	v4f32 dimensions; //x, y -> rect dims, z -> rect depths, w -> altas page
};

SpriteInstance instance_of(const Sprite& sprite, const m4x4f32& matrix) {
	return {
		matrix * glm::scale(v3f32(sprite.dimensions, 1)),
		sprite.cursor.view,
		v4f32(sprite.dimensions, sprite.depth, sprite.cursor.atlas_index)
	};
}

struct SpriteRenderer {
	struct SceneData {
		m4x4f32 view_projection;
		v4u32 atlas_dimensions;
		struct {
			u32 start;
			u32 end;
		} instances_range;
	};

	Pipeline pipeline;
	MappedBuffer<SpriteInstance> instances_buffer;
	MappedObject<SceneData> scene;
	RenderMesh rect;

	void operator()(
		Array<SpriteInstance> sprites,
		const m4x4f32& vp,
		const TexBuffer& textures
		) {
		//! will silently fail to render the latter part of the sprites array if > instance_buffer size
		u32 content_size = min(sprites.size(), instances_buffer.content.size());
		memcpy(instances_buffer.content.data(), sprites.data(), content_size * sizeof(SpriteInstance));
		sync(instances_buffer);
		sync(scene, { vp, textures.dimensions, { 0 , content_size} });
		pipeline(rect, scene.obj->instances_range.end - scene.obj->instances_range.start,
			{
				bind_to(textures, 0),
				bind_to(instances_buffer, 0),
				bind_to(scene, 0),
			}
		);
	}
};

// meant to take "ownership" of mesh, as in will clean it up upon unload call
SpriteRenderer load_sprite_renderer(const cstr pipeline_path, GLsizeiptr max_draw_batch, const RenderMesh* mesh = null) {
	return {
		load_pipeline(pipeline_path),
		map_buffer<SpriteInstance>(max_draw_batch),
		map_object<SpriteRenderer::SceneData>({}),
		mesh ? *mesh : create_rect_mesh(v2f32(1))
	};
}

void unload(SpriteRenderer& renderer) {
	destroy_pipeline(renderer.pipeline);
	unmap(renderer.instances_buffer);
	unmap(renderer.scene);
	delete_mesh(renderer.rect);
}


#endif
