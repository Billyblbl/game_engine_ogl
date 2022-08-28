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
#include <span>
#include <vector>

#include <string>

namespace Textures {

	struct ImageFormat {
		GLenum internal;
		GLenum pixel;
		GLenum elements;
	};

	template<typename T> constexpr ImageFormat ImageFormatFromElement = { 0, 0, 0 };
	template<typename T, int I, glm::qualifier Q>
	constexpr ImageFormat ImageFormatFromElement<glm::vec<I, T, Q>> = {
		Format<T>.vec[I],
		Format<T>.pixel[I],
		Format<T>.element
	};

	template<> constexpr ImageFormat ImageFormatFromElement<glm::f32> = ImageFormatFromElement<glm::f32vec1>;
	template<> constexpr ImageFormat ImageFormatFromElement<glm::i32> = ImageFormatFromElement<glm::i32vec1>;
	template<> constexpr ImageFormat ImageFormatFromElement<glm::u32> = ImageFormatFromElement<glm::u32vec1>;
	template<> constexpr ImageFormat ImageFormatFromElement<glm::i16> = ImageFormatFromElement<glm::i16vec1>;
	template<> constexpr ImageFormat ImageFormatFromElement<glm::u16> = ImageFormatFromElement<glm::u16vec1>;
	template<> constexpr ImageFormat ImageFormatFromElement<glm::i8> = ImageFormatFromElement<glm::i8vec1>;
	template<> constexpr ImageFormat ImageFormatFromElement<glm::u8> = ImageFormatFromElement<glm::u8vec1>;


	template<typename T> constexpr ImageFormat ImageFormatOf(std::span<T> source) {
		return ImageFormatFromElement<std::remove_const_t<T>>;
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
		GLuint			id;
		glm::uvec2	dimensions;
		int					channels;
	};

	Texture createFromSource(std::span<std::byte> source, const ImageFormat& format, glm::uvec2 dimensions, SamplingFilter filter = Linear, WrapMode wrap = Repeat) {
		GLuint id;

		// GL_GUARD(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

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

		// GL_GUARD(glBindTexture(GL_TEXTURE_2D, id));
		// GL_GUARD(glTexImage2D(GL_TEXTURE_2D, 0, format.internal, dimensions.x, dimensions.y, 0, format.pixel, format.elements, source.data()));

		//TODO find why that fails with an GL_INVALID_OPERATION when using the image loaded with stbi
		GL_GUARD(glTextureSubImage2D(id, 0, 0, 0, dimensions.x, dimensions.y, format.pixel, format.elements, source.data()));
		GL_GUARD(glBindTexture(GL_TEXTURE_2D, 0));

		int channels = source.size_bytes() > 0 ? source.size_bytes() / (dimensions.x * dimensions.y) : 0;
		return { id, dimensions, channels };
	}

	template<typename T, size_t E>
	Texture createFromSource(std::span<T, E> source, glm::uvec2 dimensions, SamplingFilter filter = Linear, WrapMode wrap = Repeat) {
		GLuint id;
		auto format = ImageFormatFromElement<T>;
		assert(dimensions.x * dimensions.y <= source.size());
		return createFromSource(std::span((std::byte*)source.data(), source.size_bytes()), format, dimensions, filter, wrap);
	}

	int GLtoSTBChannels(GLenum GLChannels) {
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

	constexpr Texture NullTexture = {0, glm::uvec2(0)};

	Texture loadFromFile(const char* path, const ImageFormat format = ImageFormatFromElement<glm::f32vec4>, SamplingFilter filter = Linear, WrapMode wrap = Repeat) {
		int width, height, channels = 0;
		void* img = stbi_load(path, &width, &height, &channels, GLtoSTBChannels(format.pixel));
		if (img == NULL) {
			fprintf(stderr, "Failed to load texture %s : %s\n", path, stbi_failure_reason());
			return NullTexture;
		}
		printf("Loading texture %s:%dx%d-%d\n", path, width, height, channels);

		auto dimensions = glm::uvec2(width, height);

		auto data = std::span((glm::u8*)img, width * height * channels);
		std::vector<glm::f32> converted(width * height * channels);
		for (size_t i = 0; i < data.size(); i++) converted[i] = (float)data[i] / 256;
		auto texture = createFromSource(std::span((std::byte*)converted.data(), converted.size() / channels), format, dimensions, filter, wrap);

		stbi_image_free(img);
		return texture;
	}

}


#endif
