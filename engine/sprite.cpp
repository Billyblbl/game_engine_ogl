#ifndef GSPRITE
# define GSPRITE

#include <rendering.cpp>
#include <textures.cpp>
#include <imgui_extension.cpp>
#include <image.cpp>

struct SpriteCursor {
	rtf32 uv_rect;
	u32 atlas_index;
};

bool EditorWidget(const cstr label, SpriteCursor& data) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		changed |= EditorWidget("UV rect", data.uv_rect);
		changed |= EditorWidget("Atlas index", data.atlas_index);
		ImGui::TreePop();
	}
	return changed;
}

constexpr SpriteCursor null_sprite = { { v2f32(0), v2f32(0) }, 0 };
constexpr SpriteCursor full_texture(u32 index) { return { { v2f32(0), v2f32(1) }, index }; }

TexBuffer load_texture(const cstr path, GPUFormat target_format = RGBA32F, TexType type = TX2D) {
	auto [format, dimensions, data, _] = load_image(path); defer{ stbi_image_free(data.data()); };
	if (dimensions.x * dimensions.y == 0)
		return fail_ret("can't create texture without image", null_tex);
	return create_texture(data, format, type, v4u32(dimensions, 1, 1), target_format);
}

inline rtf32 ratio(rtu32 area, v2u32 dimensions) { return { v2f32(area.min) / v2f32(dimensions), v2f32(area.max) / v2f32(dimensions) }; }

inline v2f32 rect_to_world(rtf32 rect, v2f32 v) {
	return v * (rect.max - rect.min) + rect.min;
}

inline rtf32 rect_in_rect(rtf32 reference, rtf32 rect) {
	return { rect_to_world(reference, rect.min), rect_to_world(reference, rect.max) };
}

SpriteCursor load_into(const cstr path, TexBuffer& texture, v2u32 upper_left = v2u32(0), u32 page = 0) {
	auto [format, dimensions, data, _] = load_image(path); defer{ stbi_image_free(data.data()); };
	auto rect = rtu32{ upper_left, upper_left + dimensions };
	if (data.size() > 0 &&
		expect(texture.dimensions.x >= dimensions.x && texture.dimensions.y >= dimensions.y) &&
		upload_texture_data(texture, cast<byte>(data), format, slice_to_area<2>(rect, page)))
		return SpriteCursor{ ratio(rect, texture.dimensions), page };
	else
		return {};
}

using AtlasPage = List<rtu32>;
using Atlas = Array<AtlasPage>;

//dimensions : x=width, y=height, z=page_count, w=mipmap_levels
tuple<TexBuffer, Array<rtu32>, Atlas> allocate_atlas(Alloc allocator, v4u32 dimensions, u32 max_sprite_per_page = 10) {
	auto tex = create_texture(TX2DARR, dimensions);
	auto rect_buffer = alloc_array<rtu32>(allocator, dimensions.z * max_sprite_per_page);
	auto atlas = alloc_array<AtlasPage>(allocator, dimensions.z);
	for (auto page : u64xrange{ 0, atlas.size() }) {
		atlas[page].capacity = rect_buffer.subspan(page * max_sprite_per_page, max_sprite_per_page);
		atlas[page].current = 0;
	}
	return tuple(tex, rect_buffer, atlas);
}

void dealloc_atlas(Alloc allocator, TexBuffer& texture, Array<rtu32> rects, Atlas atlas) {
	dealloc_array(allocator, atlas);
	dealloc_array(allocator, rects);
	unload(texture);
}

struct SpriteInstance {
	m4x4f32 matrix;
	rtf32 uv_rect;
	v4f32 dimensions; //x, y -> rect dims, z -> rect depths, w -> altas page
};

SpriteInstance sprite_data(m4x4f32 matrix, SpriteCursor sprite, v2f32 rect_dimensions, f32 depth) {
	//? might want to have a full transform instead of just thte rect dimensions ? would allow for more options
	return { matrix * glm::scale(v3f32(rect_dimensions, 1)), sprite.uv_rect, v4f32(rect_dimensions, depth, sprite.atlas_index) };
}

struct SpriteRenderer {
	struct SceneData {
		m4x4f32 view_projection;
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
		sync(scene, {vp, { 0 , content_size}});
		pipeline(rect, scene.obj->instances_range.end - scene.obj->instances_range.start, {
			bind_to(textures, 0),
			bind_to(instances_buffer, 0),
			bind_to(scene, 0),
		});
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
