#ifndef GIMAGE
# define GIMAGE

#include <math.cpp>
#include <glutils.cpp>
#include <blblstd.hpp>

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

struct Image {
	SrcFormat format;
	v2u32 dimensions;
	Buffer data;
	u64 pixel_size;

	u64 pixel_index(v2u64 coord) const { return pixel_size * (coord.x + coord.y * dimensions.x); }
	Buffer pixel(v2u64 coord) { return data.subspan(pixel_index(coord), pixel_size); }
	Array<const byte> pixel(v2u64 coord) const { return data.subspan(pixel_index(coord), pixel_size); }
	Buffer operator[](v2u64 coord) { return pixel(coord); };
	Array<const byte> operator[](v2u64 coord) const { return pixel(coord); };
};

template<typename T> inline Image make_image(Array<T> source, v2u32 dimensions, u32 channels) {
	return { Formats<T>[channels], dimensions, cast<byte>(source), channels * sizeof(T) };
}

template<typename T, i32 D> inline Image make_image(Array<const glm::vec<D, T>> source, v2u32 dimensions) {
	return { Formats<T>[D], dimensions, cast<byte>(source), D * sizeof(T) };
}

Image load_image(const cstr path) {
	int width, height, channels = 0;
	auto* img = stbi_load(path, &width, &height, &channels, 0);
	printf("Loading image %s:%ix%i-%i\n", path, width, height, channels);
	if (img == null)
		return fail_ret(stbi_failure_reason(), (Image{ SrcFormat{}, v2u32(0), Array<u8>{}, 0 }));
	return make_image<u8>(carray((byte*)img, width * height * channels), v2u32(width, height), channels);
}

//! should only be given an image loaded with load_image
Image& unload(Image& img) {
	stbi_image_free(img.data.data());
	img.format = {};
	img.dimensions = v2u32(0);
	img.data = {};
	img.pixel_size = 0;
	return img;
}

#endif
