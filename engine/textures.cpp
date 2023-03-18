#ifndef GTEXTURES
# define GTEXTURES

#include <glm/glm.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <GL/glext.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
// #include <GL/glext.h>
#include <glutils.cpp>
#include <string>

namespace Textures {

	struct ImageFormat {
		GLenum internal;
		GLenum pixel;
		GLenum elements;
	};

	template<typename T> constexpr ImageFormat image_format_from_element(int channels) {
		return {
			Format<T>.vec[channels],
			Format<T>.pixel[channels],
			Format<T>.element
		};
	}

	template<typename T> constexpr ImageFormat ImageFormatFromPixelType = { 0, 0, 0 };
	template<typename T, int I, glm::qualifier Q>
	constexpr ImageFormat ImageFormatFromPixelType<glm::vec<I, T, Q>> = image_format_from_element<T>(I);

	template<> constexpr ImageFormat ImageFormatFromPixelType<f32> = ImageFormatFromPixelType<v1f32>;
	template<> constexpr ImageFormat ImageFormatFromPixelType<i32> = ImageFormatFromPixelType<v1i32>;
	template<> constexpr ImageFormat ImageFormatFromPixelType<u32> = ImageFormatFromPixelType<v1u32>;
	template<> constexpr ImageFormat ImageFormatFromPixelType<i16> = ImageFormatFromPixelType<v1i16>;
	template<> constexpr ImageFormat ImageFormatFromPixelType<u16> = ImageFormatFromPixelType<v1u16>;
	template<> constexpr ImageFormat ImageFormatFromPixelType<i8> = ImageFormatFromPixelType<v1i8>;
	template<> constexpr ImageFormat ImageFormatFromPixelType<u8> = ImageFormatFromPixelType<v1u8>;


	template<typename T> constexpr ImageFormat image_format_of(std::span<T> source) {
		return ImageFormatFromPixelType<std::remove_const_t<T>>;
	}

	enum SamplingFilter : GLenum {
		Nearest = GL_NEAREST,
		Linear = GL_LINEAR
	};

	enum WrapMode : GLenum {
		Clamp = GL_CLAMP_TO_EDGE,
		MirroredRepeat = GL_MIRRORED_REPEAT,
		Repeat = GL_REPEAT
	};

	struct Texture {
		GLuint	id;
		v2u32		dimensions;
		i32			channels;
	};

	Texture allocate(const ImageFormat& format, glm::uvec2 dimensions, SamplingFilter filter = Linear, WrapMode wrap = Repeat) {
		GLuint id;
		GL_GUARD(glBindTexture(GL_TEXTURE_2D, 0));
		GL_GUARD(glCreateTextures(GL_TEXTURE_2D, 1, &id));
		printf("Creating Texture %u\n", id);
		fflush(stdout);
		GL_GUARD(glTextureStorage2D(id, 1, format.internal, dimensions.x, dimensions.y));
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, filter));
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, filter));
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_WRAP_R, wrap));
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap));
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap));
		return { id, dimensions, 0 };
	}

	Texture create_from_source(Array<byte> source, const ImageFormat& format, v2u32 dimensions, SamplingFilter filter = Linear, WrapMode wrap = Repeat) {

		auto texture = allocate(format, dimensions, filter, wrap);

		//TODO find why that fails with an GL_INVALID_OPERATION when using the image loaded with stbi
		GL_GUARD(glTextureSubImage2D(texture.id, 0, 0, 0, dimensions.x, dimensions.y, format.pixel, format.elements, source.data()));
		GL_GUARD(glBindTexture(GL_TEXTURE_2D, 0));

		texture.channels = source.size_bytes() > 0 ? source.size_bytes() / (dimensions.x * dimensions.y) : 0;
		return texture;
	}

	template<typename T> Texture create_from_source(Array<T> source, v2u32 dimensions, SamplingFilter filter = Linear, WrapMode wrap = Repeat) {
		GLuint id;
		auto format = ImageFormatFromPixelType<T>;
		assert(dimensions.x * dimensions.y <= source.size());
		return create_from_source(Array((byte*)source.data(), source.size_bytes()), format, dimensions, filter, wrap);
	}

	int GL_to_STB_channels(GLenum GLChannels) {
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

	constexpr Texture NullTexture = { 0, v2u32(0) };

	Texture load_from_file(const cstr path, SamplingFilter filter = Linear, WrapMode wrap = Repeat) {
		int width, height, channels = 0;
		auto* img = stbi_load(path, &width, &height, &channels, 0);
		if (img == null) {
			fprintf(stderr, "Failed to load texture %s : %s\n", path, stbi_failure_reason());
			return NullTexture;
		}
		printf("Loading texture %s:%dx%d-%d\n", path, width, height, channels);

		auto format = image_format_from_element<f32>(channels);
		auto dimensions = v2u32(width, height);
		auto data = Array<u8>((u8*)img, width * height * channels);

		//TODO might be problematic since its the size of the texture, could easily be too big
		f32 converted[width * height * channels];
		for (u64 i = 0; i < data.size(); i++)
			converted[i] = (f32)data[i] / 256.f;
		auto texture = create_from_source(Array<byte>((byte*)&converted[0], width * height * channels * sizeof(f32)), format, dimensions, filter, wrap);

		stbi_image_free(img);
		return texture;
	}

}


#endif
