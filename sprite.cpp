#ifndef GSPRITE
# define GSPRITE

#include <rendering.cpp>
#include <textures.cpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int gl_to_stb_channels(GLenum GLChannels) {
	switch (GLChannels) {
	case GL_RED: return STBI_grey;
	case GL_RG: return STBI_grey_alpha;
	case GL_RGB: return STBI_rgb;
	case GL_RGBA: return STBI_rgb_alpha;
	case GL_RED_INTEGER: return STBI_grey;
	case GL_RG_INTEGER: return STBI_grey_alpha;
	case GL_RGB_INTEGER: return STBI_rgb;
	case GL_RGBA_INTEGER: return STBI_rgb_alpha;
	default: return 0;
	}
}

struct SpriteIndex {
	rtf32 uv_rect;
	u32 atlas_index;
};

constexpr SpriteIndex null_sprite = { { v2f32(0), v2f32(0) }, 0 };
constexpr SpriteIndex full_texture(u32 index) { return { { v2f32(0), v2f32(1) }, index }; }

tuple<SrcFormat, v2u32, Array<u8>> load_image(const cstr path) {
	int width, height, channels = 0;
	auto* img = stbi_load(path, &width, &height, &channels, 0);
	if (img == null)
		return fail_ret(stbi_failure_reason(), tuple(SrcFormat{}, v2u32(0), Array<u8>{}));
	printf("Loaded texture %s:%ix%i-%i\n", path, width, height, channels);
	return tuple(Formats<u8>[channels], v2u32(width, height), carray((u8*)img, width * height * channels));
}

TexBuffer load_texture(const cstr path, GPUFormat target_format = RGBA32F) {
	auto [format, dimensions, data] = load_image(path); defer{ stbi_image_free(data.data()); };
	return create_texture(data, format, TX2D, v4u32(dimensions, 1, 1), target_format);
}

rtf32 ratio(rtu32 area, v2u32 dimensions) { return { v2f32(area.min) / v2f32(dimensions), v2f32(area.max) / v2f32(dimensions) }; }

SpriteIndex load_into(const cstr path, TexBuffer& texture, v2u32 upper_left, u32 index = 0) {
	auto [format, dimensions, data] = load_image(path); defer{ stbi_image_free(data.data()); };
	auto rect = rtu32{ upper_left, upper_left + dimensions };
	if (data.size() > 0 &&
		expect(texture.dimensions.x >= dimensions.x && texture.dimensions.y >= dimensions.y) &&
		upload_texture_data(texture, cast<byte>(data), format, slice_to_area<2>(rect, index)))
		return SpriteIndex{ ratio(rect, texture.dimensions), index };
	else
		return {};
}

//Rectangle
// rtf32 fit(v2u32 dimensions, v2u32 storage, Array<rtf32> used) {
// 	//TODO Implement
// }

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
	v2u64 atlas_index;
};

struct SpriteRenderer {
	Pipeline pipeline;
	MappedBuffer<SpriteInstance> instances_buffer;

	void operator()(
		const RenderMesh& mesh,
		const TexBuffer& textures,
		MappedObject<m4x4f32> matrix,
		u32 instance_count
		) {
		sync(matrix);
		sync(instances_buffer);
		pipeline(mesh, instance_count, {
			bind_to(textures, 0),
			bind_to(instances_buffer, 0),
			bind_to(matrix, 0)
		});
	}

	List<SpriteInstance> start_batch() { return List { instances_buffer.content, 0 }; }
};

SpriteRenderer load_sprite_renderer(const cstr pipeline_path, GLsizeiptr max_draw_batch) {
	return { load_pipeline(pipeline_path), map_buffer<SpriteInstance>(max_draw_batch)};
}

void unload(SpriteRenderer& renderer) {
	destroy_pipeline(renderer.pipeline);
	unmap(renderer.instances_buffer);
}


#endif