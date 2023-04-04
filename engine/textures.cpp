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
#include <math.cpp>

enum SamplingFilter: GLint {
	Nearest = GL_NEAREST,
	Linear = GL_LINEAR,
	NearestMipmapNearest = GL_NEAREST_MIPMAP_NEAREST,
	LinearMipmapNearest = GL_LINEAR_MIPMAP_NEAREST,
	NearestMipmapLinear = GL_NEAREST_MIPMAP_LINEAR,
	LinearMipmapLinear = GL_LINEAR_MIPMAP_LINEAR
};

enum WrapMode: GLint {
	Clamp = GL_CLAMP_TO_EDGE,
	MirroredRepeat = GL_MIRRORED_REPEAT,
	Repeat = GL_REPEAT,
	ClampToBorder = GL_CLAMP_TO_BORDER,
	MirroredClamp = GL_MIRROR_CLAMP_TO_EDGE
};

enum DSTextureMode: GLint {
	Depth = GL_DEPTH_COMPONENT,
	Stencil = GL_STENCIL_INDEX
};

enum Comparison: GLint {
	None = 0,
	LEq = GL_LEQUAL,
	GEq = GL_GEQUAL,
	Less = GL_LESS,
	Greater = GL_GREATER,
	Eq = GL_EQUAL,
	NEq = GL_NOTEQUAL,
	Always = GL_ALWAYS,
	Never = GL_NEVER
};

enum Swizzle: GLint {
	Red = GL_RED,
	Green = GL_GREEN,
	Blue = GL_BLUE,
	Alpha = GL_ALPHA,
	One = GL_ONE,
	Zero = GL_ZERO,
};

enum TexType: GLenum {
	NoType = 0,
	TX1D = GL_TEXTURE_1D,
	TX2D = GL_TEXTURE_2D,
	TX3D = GL_TEXTURE_3D,
	TX1DARR = GL_TEXTURE_1D_ARRAY,
	TX2DARR = GL_TEXTURE_2D_ARRAY,
	TXRECT = GL_TEXTURE_RECTANGLE,
	TXCMAP = GL_TEXTURE_CUBE_MAP,
	TXCMAPARR = GL_TEXTURE_CUBE_MAP_ARRAY,
	TXBUFF = GL_TEXTURE_BUFFER,
	TX2DMULTI = GL_TEXTURE_2D_MULTISAMPLE,
	TX2DMULTIARR = GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
};

using SwizzleConfig = glm::vec<4, Swizzle>;
using WrapConfig = glm::vec<3, WrapMode>;
struct SamplingConfig { SamplingFilter min, max; };

template<u32 D> using Area = polytope<glm::vec<D, u32>>;

struct TexBuffer {
	GLuint	id;
	v4u32		dimensions;
	TexType type;
	GPUFormat format;

	TexBuffer& conf_wrap(WrapConfig wrap) {
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_WRAP_S, wrap.s));
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_WRAP_T, wrap.t));
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_WRAP_R, wrap.r));
		return *this;
	}

	TexBuffer& conf_sampling(SamplingConfig filter) {
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, filter.min));
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, filter.max));
		return *this;
	}

	TexBuffer& conf_stencil_mode(DSTextureMode mode) {
		GL_GUARD(glTextureParameteri(id, GL_DEPTH_STENCIL_TEXTURE_MODE, mode));
		return *this;
	}

	TexBuffer& conf_border_color(v3f32 color) {
		GL_GUARD(glTextureParameterfv(id, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(color)));
		return *this;
	}

	TexBuffer& conf_border_color(v3i32 color) {
		GL_GUARD(glTextureParameteriv(id, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(color)));
		return *this;
	}

	TexBuffer& conf_border_color(u32 color) {
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_BORDER_COLOR, color));
		return *this;
	}

	TexBuffer& conf_compare(Comparison comp) {
		if (comp == None) {
			GL_GUARD(glTextureParameteri(id, GL_TEXTURE_COMPARE_MODE, GL_NONE));
		} else {
			GL_GUARD(glTextureParameteri(id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE));
			GL_GUARD(glTextureParameteri(id, GL_TEXTURE_COMPARE_FUNC, comp));
		}
		return *this;
	}

	TexBuffer& conf_lod(f32range range = { -1000, 1000 }, f32 bias = 0) {
		GL_GUARD(glTextureParameterf(id, GL_TEXTURE_LOD_BIAS, bias));
		GL_GUARD(glTextureParameterf(id, GL_TEXTURE_MIN_LOD, range.min));
		GL_GUARD(glTextureParameterf(id, GL_TEXTURE_MAX_LOD, range.max));
		return *this;
	}

	TexBuffer& conf_levels(u32range range = { 0, 1000 }) {
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_BASE_LEVEL, range.min));
		GL_GUARD(glTextureParameteri(id, GL_TEXTURE_MAX_LEVEL, range.max));
		return *this;
	}

	TexBuffer& conf_swizzle(SwizzleConfig swizzle) {
		GL_GUARD(glTextureParameteriv(id, GL_TEXTURE_SWIZZLE_RGBA, (int*)&swizzle));
		return *this;
	}

};

constexpr TexBuffer null_tex = { 0, v4u32(0), NoType };

TexBuffer create_texture(TexType type, v4u32 dimensions, GPUFormat format = RGBA32F) {
	GLuint id;
	GL_GUARD(glCreateTextures(type, 1, &id));
	printf("Creating %s Texture %u\n", GLtoString(type).data(), id);
	fflush(stdout);
	switch (type) {
	case TX1D:		GL_GUARD(glTextureStorage1D(id, dimensions.w, format, dimensions.x)); break;
	case TX2D:		GL_GUARD(glTextureStorage2D(id, dimensions.w, format, dimensions.x, dimensions.z)); break;
	case TX1DARR:	GL_GUARD(glTextureStorage2D(id, dimensions.w, format, dimensions.x, dimensions.z)); break;
	case TX3D:		GL_GUARD(glTextureStorage3D(id, dimensions.w, format, dimensions.x, dimensions.y, dimensions.z)); break;
	case TX2DARR:	GL_GUARD(glTextureStorage3D(id, dimensions.w, format, dimensions.x, dimensions.y, dimensions.z)); break;
	default: return fail_ret(GLtoString(type).data(), null_tex);
	}
	return { id, dimensions, type, format };
}

void unload(TexBuffer& texture) {
	GL_GUARD(glDeleteTextures(1, &texture.id));
	texture = null_tex;
}

bool upload_texture_data(TexBuffer& texture, Array<byte> source, SrcFormat format, Area<3> box) {
	switch (texture.type) {
	case TX1D:		GL_GUARD(glTextureSubImage1D(texture.id, 0, box.min.x, width(box), format.channels, format.type, source.data())); break;
	case TX2D:		GL_GUARD(glTextureSubImage2D(texture.id, 0, box.min.x, box.min.y, width(box), height(box), format.channels, format.type, source.data())); break;
	case TX1DARR:	GL_GUARD(glTextureSubImage2D(texture.id, 0, box.min.x, box.min.y, width(box), height(box), format.channels, format.type, source.data())); break;
	case TX3D:		GL_GUARD(glTextureSubImage3D(texture.id, 0, box.min.x, box.min.y, box.min.z, width(box), height(box), depth(box), format.channels, format.type, source.data())); break;
	case TX2DARR:	GL_GUARD(glTextureSubImage3D(texture.id, 0, box.min.x, box.min.y, box.min.z, width(box), height(box), depth(box), format.channels, format.type, source.data())); break;
	default: return fail_ret(GLtoString(texture.type).data(), false);
	}
	return true;
}

template<typename T, u32 D> bool upload_texture_data(TexBuffer& texture, Array<T> source, Area<D> area) {
	//TODO proper area fit check
	// expect(glm::length2(area.max - area.min) <= source.size());
	return upload_texture_data(texture, cast<byte>(source), Format<T>, area);
}

template<typename T, u32 D> bool upload_texture_data(TexBuffer& texture, Array<T> source, Area<D> area, u32 index) {
	return upload_texture_data(texture, source, Area<D + 1> { { area.min, index }, { area.max, index + 1 } });
}

TexBuffer create_texture(Array<byte> source, SrcFormat source_format, TexType type, v4u32 dimensions, GPUFormat internal_format = RGBA32F) {
	auto texture = create_texture(type, dimensions, internal_format);
	if (upload_texture_data(texture, source, source_format, bxu32{ v3u32(0), dimensions }))
		return texture;
	else {
		unload(texture);
		return fail_ret("texture creation failed", null_tex);
	}
}

template<typename T> TexBuffer create_texture(Array<T> source, TexType type, v4u32 dimensions, GPUFormat internal_format = RGBA32F) {
	expect(dimensions.x * dimensions.y * dimensions.z <= source.size());
	return create_texture(cast<byte>(source), Format<T>, type, dimensions, internal_format);
}

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

#endif
