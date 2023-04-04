#ifndef GSPRITE
# define GSPRITE

#include <rendering.cpp>
#include <textures.cpp>

struct Sprite {
	TexBuffer* source;
	bxf32 area;
};

tuple<SrcFormat, v2u32, Array<u8>> load_image(const cstr path) {
	int width, height, channels = 0;
	auto* img = stbi_load(path, &width, &height, &channels, 0);
	if (img == null)
		fail_ret(stbi_failure_reason(), tuple(SrcFormat{}, v2u32(0), Array<u8>{}));
	// printf("Loaded texture %s:%dx%d-%d\n", path, width, height, channels);
	return tuple(Formats<u8>[channels], v2u32(width, height), carray((u8*)img, width * height * channels));
}

TexBuffer load_texture(const cstr path, GPUFormat target_format = RGBA32F) {
	auto [format, dimensions, data] = load_image(path); defer{ stbi_image_free(data.data()); };
	return create_texture(data, format, TX2D, v4u32(dimensions, 1, 1), target_format);
}

Area<3> load_into(const cstr path, TexBuffer& texture, u32 index) {
	printf("Loading %s into index %i of texture buffer %u\n", path, index, texture.id); fflush(stdout);
	auto [format, dimensions, data] = load_image(path); defer{ stbi_image_free(data.data()); };
	expect(texture.dimensions.x >= dimensions.x && texture.dimensions.y >= dimensions.y);
	auto target_area = Area<3>{ v3u32(0, 0, index), v3u32(dimensions, index + 1) };
	if (upload_texture_data(texture, cast<byte>(data), format, target_area))
		return target_area;
	else
		return { v3u32(0), v3u32(0) };
}

struct BatchTarget {
	RenderMesh* mesh;
	TexBuffer* atlas;
};

struct Atlas {
	MappedBuffer<rtf32> used_buffer;
	List<rtf32> used;
	TexBuffer textures;

	rtf32 ratio(rtu32 area) { return { v2f32(area.min) / v2f32(textures.dimensions), v2f32(area.max) / v2f32(textures.dimensions) }; }

	u32 push(Array<byte> source, SrcFormat format, rtu32 rect) {
		upload_texture_data(textures, source, format, bxu32{ v3u32(rect.min, used.current), v3u32(rect.max, used.current + 1) });
		auto index = used.current;
		used.push(ratio(rect));
		return index;
	}

	template<typename T> u32 push(Array<T> source, rtu32 rect) {
		return push(cast<byte>(source), Format<T>, rect);
	}

	u32 load(const cstr path) {
		auto area = load_into(path, textures, used.current);
		auto index = used.current;
		used.push(ratio(area));
		return index;
	}

};

// Dimension : x -> width, y -> height, z -> number of pages, w -> mipmaps levels
Atlas allocate_atlas(v4u32 dimensions, GPUFormat format = RGBA32F) {
	Atlas atlas;
	atlas.used_buffer = map_buffer<rtf32>(dimensions.z);
	atlas.textures = create_texture(TX2DARR, dimensions, format);
	atlas.used = List{ atlas.used_buffer.content, 0 };
	return atlas;
}

void dealloc_atlas(Atlas& atlas) {
	unload(atlas.textures);
	unmap(atlas.used_buffer);
	atlas.used = { {}, 0 };
}

#endif
