#ifndef GGLUTILS
# define GGLUTILS

#include <blblstd.hpp>
#include <cstdio>
#include <optional>
#include <math.cpp>
#define GLFW_INCLUDE_GLEXT
#include <GL/glew.h>
#include <GL/glcorearb.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#define serialise_macro(d) lstr(#d)
#define case_macro(d) case d: return lstr(#d)

const string GLtoString(GLenum value) {
	switch (value) {
	case_macro(GL_TEXTURE_1D);
	case_macro(GL_TEXTURE_2D);
	case_macro(GL_TEXTURE_3D);
	case_macro(GL_TEXTURE_1D_ARRAY);
	case_macro(GL_TEXTURE_2D_ARRAY);
	case_macro(GL_TEXTURE_RECTANGLE);
	case_macro(GL_TEXTURE_CUBE_MAP);
	case_macro(GL_TEXTURE_CUBE_MAP_ARRAY);
	case_macro(GL_TEXTURE_BUFFER);
	case_macro(GL_TEXTURE_2D_MULTISAMPLE);
	case_macro(GL_TEXTURE_2D_MULTISAMPLE_ARRAY);
	case_macro(GL_FRAGMENT_SHADER);
	case_macro(GL_VERTEX_SHADER);
	case_macro(GL_DEPTH_BUFFER_BIT);
	case_macro(GL_STENCIL_BUFFER_BIT);
	case_macro(GL_COLOR_BUFFER_BIT);
	case_macro(GL_POINTS);
	case_macro(GL_LINES);
	case_macro(GL_LINE_LOOP);
	case_macro(GL_LINE_STRIP);
	case_macro(GL_TRIANGLES);
	case_macro(GL_TRIANGLE_STRIP);
	case_macro(GL_TRIANGLE_FAN);
	case_macro(GL_QUADS);
	case_macro(GL_NEVER);
	case_macro(GL_LESS);
	case_macro(GL_EQUAL);
	case_macro(GL_LEQUAL);
	case_macro(GL_GREATER);
	case_macro(GL_NOTEQUAL);
	case_macro(GL_GEQUAL);
	case_macro(GL_ALWAYS);
	case_macro(GL_SRC_COLOR);
	case_macro(GL_ONE_MINUS_SRC_COLOR);
	case_macro(GL_SRC_ALPHA);
	case_macro(GL_ONE_MINUS_SRC_ALPHA);
	case_macro(GL_DST_ALPHA);
	case_macro(GL_ONE_MINUS_DST_ALPHA);
	case_macro(GL_DST_COLOR);
	case_macro(GL_ONE_MINUS_DST_COLOR);
	case_macro(GL_SRC_ALPHA_SATURATE);
	case_macro(GL_FRONT_RIGHT);
	case_macro(GL_BACK_LEFT);
	case_macro(GL_BACK_RIGHT);
	case_macro(GL_FRONT);
	case_macro(GL_BACK);
	case_macro(GL_LEFT);
	case_macro(GL_RIGHT);
	case_macro(GL_FRONT_AND_BACK);
	case_macro(GL_INVALID_ENUM);
	case_macro(GL_INVALID_VALUE);
	case_macro(GL_INVALID_OPERATION);
	case_macro(GL_OUT_OF_MEMORY);
	case_macro(GL_CW);
	case_macro(GL_CCW);
	case_macro(GL_POINT_SIZE);
	case_macro(GL_POINT_SIZE_RANGE);
	case_macro(GL_POINT_SIZE_GRANULARITY);
	case_macro(GL_LINE_SMOOTH);
	case_macro(GL_LINE_WIDTH);
	case_macro(GL_LINE_WIDTH_RANGE);
	case_macro(GL_LINE_WIDTH_GRANULARITY);
	case_macro(GL_POLYGON_MODE);
	case_macro(GL_POLYGON_SMOOTH);
	case_macro(GL_CULL_FACE);
	case_macro(GL_CULL_FACE_MODE);
	case_macro(GL_FRONT_FACE);
	case_macro(GL_DEPTH_RANGE);
	case_macro(GL_DEPTH_TEST);
	case_macro(GL_DEPTH_WRITEMASK);
	case_macro(GL_DEPTH_CLEAR_VALUE);
	case_macro(GL_DEPTH_FUNC);
	case_macro(GL_STENCIL_TEST);
	case_macro(GL_STENCIL_CLEAR_VALUE);
	case_macro(GL_STENCIL_FUNC);
	case_macro(GL_STENCIL_VALUE_MASK);
	case_macro(GL_STENCIL_FAIL);
	case_macro(GL_STENCIL_PASS_DEPTH_FAIL);
	case_macro(GL_STENCIL_PASS_DEPTH_PASS);
	case_macro(GL_STENCIL_REF);
	case_macro(GL_STENCIL_WRITEMASK);
	case_macro(GL_VIEWPORT);
	case_macro(GL_DITHER);
	case_macro(GL_BLEND_DST);
	case_macro(GL_BLEND_SRC);
	case_macro(GL_BLEND);
	case_macro(GL_LOGIC_OP_MODE);
	case_macro(GL_DRAW_BUFFER);
	case_macro(GL_READ_BUFFER);
	case_macro(GL_SCISSOR_BOX);
	case_macro(GL_SCISSOR_TEST);
	case_macro(GL_COLOR_CLEAR_VALUE);
	case_macro(GL_COLOR_WRITEMASK);
	case_macro(GL_DOUBLEBUFFER);
	case_macro(GL_STEREO);
	case_macro(GL_LINE_SMOOTH_HINT);
	case_macro(GL_POLYGON_SMOOTH_HINT);
	case_macro(GL_UNPACK_SWAP_BYTES);
	case_macro(GL_UNPACK_LSB_FIRST);
	case_macro(GL_UNPACK_ROW_LENGTH);
	case_macro(GL_UNPACK_SKIP_ROWS);
	case_macro(GL_UNPACK_SKIP_PIXELS);
	case_macro(GL_UNPACK_ALIGNMENT);
	case_macro(GL_PACK_SWAP_BYTES);
	case_macro(GL_PACK_LSB_FIRST);
	case_macro(GL_PACK_ROW_LENGTH);
	case_macro(GL_PACK_SKIP_ROWS);
	case_macro(GL_PACK_SKIP_PIXELS);
	case_macro(GL_PACK_ALIGNMENT);
	case_macro(GL_MAX_TEXTURE_SIZE);
	case_macro(GL_MAX_VIEWPORT_DIMS);
	case_macro(GL_SUBPIXEL_BITS);
	case_macro(GL_TEXTURE_WIDTH);
	case_macro(GL_TEXTURE_HEIGHT);
	case_macro(GL_TEXTURE_BORDER_COLOR);
	case_macro(GL_DONT_CARE);
	case_macro(GL_FASTEST);
	case_macro(GL_NICEST);
	case_macro(GL_BYTE);
	case_macro(GL_UNSIGNED_BYTE);
	case_macro(GL_SHORT);
	case_macro(GL_UNSIGNED_SHORT);
	case_macro(GL_INT);
	case_macro(GL_UNSIGNED_INT);
	case_macro(GL_FLOAT);
	case_macro(GL_STACK_OVERFLOW);
	case_macro(GL_STACK_UNDERFLOW);
	case_macro(GL_CLEAR);
	case_macro(GL_AND);
	case_macro(GL_AND_REVERSE);
	case_macro(GL_COPY);
	case_macro(GL_AND_INVERTED);
	case_macro(GL_NOOP);
	case_macro(GL_XOR);
	case_macro(GL_OR);
	case_macro(GL_NOR);
	case_macro(GL_EQUIV);
	case_macro(GL_INVERT);
	case_macro(GL_OR_REVERSE);
	case_macro(GL_COPY_INVERTED);
	case_macro(GL_OR_INVERTED);
	case_macro(GL_NAND);
	case_macro(GL_SET);
	case_macro(GL_TEXTURE);
	case_macro(GL_COLOR);
	case_macro(GL_DEPTH);
	case_macro(GL_STENCIL);
	case_macro(GL_STENCIL_INDEX);
	case_macro(GL_DEPTH_COMPONENT);
	case_macro(GL_RED);
	case_macro(GL_GREEN);
	case_macro(GL_BLUE);
	case_macro(GL_ALPHA);
	case_macro(GL_RGB);
	case_macro(GL_RGBA);
	case_macro(GL_POINT);
	case_macro(GL_LINE);
	case_macro(GL_FILL);
	case_macro(GL_KEEP);
	case_macro(GL_REPLACE);
	case_macro(GL_INCR);
	case_macro(GL_DECR);
	case_macro(GL_VENDOR);
	case_macro(GL_RENDERER);
	case_macro(GL_VERSION);
	case_macro(GL_EXTENSIONS);
	case_macro(GL_NEAREST);
	case_macro(GL_LINEAR);
	case_macro(GL_NEAREST_MIPMAP_NEAREST);
	case_macro(GL_LINEAR_MIPMAP_NEAREST);
	case_macro(GL_NEAREST_MIPMAP_LINEAR);
	case_macro(GL_LINEAR_MIPMAP_LINEAR);
	case_macro(GL_TEXTURE_MAG_FILTER);
	case_macro(GL_TEXTURE_MIN_FILTER);
	case_macro(GL_TEXTURE_WRAP_S);
	case_macro(GL_TEXTURE_WRAP_T);
	case_macro(GL_REPEAT);
	case_macro(GL_DEBUG_SOURCE_API);
	case_macro(GL_DEBUG_SOURCE_WINDOW_SYSTEM);
	case_macro(GL_DEBUG_SOURCE_SHADER_COMPILER);
	case_macro(GL_DEBUG_SOURCE_THIRD_PARTY);
	case_macro(GL_DEBUG_SOURCE_APPLICATION);
	case_macro(GL_DEBUG_SOURCE_OTHER);
	case_macro(GL_DEBUG_TYPE_ERROR);

	case_macro(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR);
	case_macro(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR);
	case_macro(GL_DEBUG_TYPE_PORTABILITY);
	case_macro(GL_DEBUG_TYPE_PERFORMANCE);
	case_macro(GL_DEBUG_TYPE_MARKER);
	case_macro(GL_DEBUG_TYPE_PUSH_GROUP);
	case_macro(GL_DEBUG_TYPE_POP_GROUP);
	case_macro(GL_DEBUG_TYPE_OTHER);

	case_macro(GL_DEBUG_SEVERITY_HIGH);
	case_macro(GL_DEBUG_SEVERITY_MEDIUM);
	case_macro(GL_DEBUG_SEVERITY_LOW);
	case_macro(GL_DEBUG_SEVERITY_NOTIFICATION);

	case_macro(GL_FRAMEBUFFER_UNDEFINED);
	case_macro(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
	case_macro(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
	case_macro(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
	case_macro(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
	case_macro(GL_FRAMEBUFFER_UNSUPPORTED);
	case_macro(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
	case_macro(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);
	case_macro(GL_FRAMEBUFFER_COMPLETE);

	case_macro(GL_DEPTH_COMPONENT16);
	case_macro(GL_DEPTH_COMPONENT24);
	case_macro(GL_DEPTH_COMPONENT32);
	case_macro(GL_R8);
	case_macro(GL_R8_SNORM);
	case_macro(GL_R16);
	case_macro(GL_R16_SNORM);
	case_macro(GL_RG8);
	case_macro(GL_RG8_SNORM);
	case_macro(GL_RG16);
	case_macro(GL_RG16_SNORM);
	case_macro(GL_R3_G3_B2);
	case_macro(GL_RGB4);
	case_macro(GL_RGB5);
	case_macro(GL_RGB8);
	case_macro(GL_RGB8_SNORM);
	case_macro(GL_RGB10);
	case_macro(GL_RGB12);
	case_macro(GL_RGB16_SNORM);
	case_macro(GL_RGBA2);
	case_macro(GL_RGBA4);
	case_macro(GL_RGB5_A1);
	case_macro(GL_RGBA8);
	case_macro(GL_RGBA8_SNORM);
	case_macro(GL_RGB10_A2);
	case_macro(GL_RGB10_A2UI);
	case_macro(GL_RGBA12);
	case_macro(GL_RGBA16);
	case_macro(GL_SRGB8);
	case_macro(GL_SRGB8_ALPHA8);
	case_macro(GL_R16F);
	case_macro(GL_RG16F);
	case_macro(GL_RGB16F);
	case_macro(GL_RGBA16F);
	case_macro(GL_R32F);
	case_macro(GL_RG32F);
	case_macro(GL_RGB32F);
	case_macro(GL_RGBA32F);
	case_macro(GL_R11F_G11F_B10F);
	case_macro(GL_RGB9_E5);
	case_macro(GL_R8I);
	case_macro(GL_R8UI);
	case_macro(GL_R16I);
	case_macro(GL_R16UI);
	case_macro(GL_R32I);
	case_macro(GL_R32UI);
	case_macro(GL_RG8I);
	case_macro(GL_RG8UI);
	case_macro(GL_RG16I);
	case_macro(GL_RG16UI);
	case_macro(GL_RG32I);
	case_macro(GL_RG32UI);
	case_macro(GL_RGB8I);
	case_macro(GL_RGB8UI);
	case_macro(GL_RGB16I);
	case_macro(GL_RGB16UI);
	case_macro(GL_RGB32I);
	case_macro(GL_RGB32UI);
	case_macro(GL_RGBA8I);
	case_macro(GL_RGBA8UI);
	case_macro(GL_RGBA16I);
	case_macro(GL_RGBA16UI);
	case_macro(GL_RGBA32I);
	case_macro(GL_RGBA32UI);

	case_macro(GL_RED_INTEGER);
	case_macro(GL_RG_INTEGER);
	case_macro(GL_RGB_INTEGER);
	case_macro(GL_RGBA_INTEGER);

	default: return lstr("Unknown OpenGL enum");
	}
}

void CheckGLError(string expression, string file_name, u32 line_number) {
	for (auto err = glGetError(); err != GL_NO_ERROR; err = glGetError()) {
		fprintf(stderr, "Error in file %s:%d, when executing %s : %x %s\n", file_name.data(), line_number, expression.data(), err, GLtoString(err).data());
		err = 0;
	}
}

#define DEBUG_GL true
// #define DEBUG_GL false
// #define DEBUG_GL_GUARD DEBUG_GL
#define DEBUG_GL_GUARD false

#if DEBUG_GL_GUARD
#define GL_GUARD(x) [&]() -> auto { defer {CheckGLError(#x, __FILE__, __LINE__);}; return x;}()
#else
#define GL_GUARD(x) x
#endif


enum GPUFormat : GLenum {
	NONE = 0,
	DEPTH_COMPONENT = GL_DEPTH_COMPONENT,
	DEPTH_STENCIL = GL_DEPTH_STENCIL,
	RED = GL_RED,
	RG = GL_RG,
	RGB = GL_RGB,
	RGBA = GL_RGBA,

	//Sized
	DEPTH_COMPONENT16 = GL_DEPTH_COMPONENT16,
	DEPTH_COMPONENT24 = GL_DEPTH_COMPONENT24,
	DEPTH_COMPONENT32 = GL_DEPTH_COMPONENT32,
	R8 = GL_R8,
	R8_SNORM = GL_R8_SNORM,
	R16 = GL_R16,
	R16_SNORM = GL_R16_SNORM,
	RG8 = GL_RG8,
	RG8_SNORM = GL_RG8_SNORM,
	RG16 = GL_RG16,
	RG16_SNORM = GL_RG16_SNORM,
	R3_G3_B2 = GL_R3_G3_B2,
	RGB4 = GL_RGB4,
	RGB5 = GL_RGB5,
	RGB8 = GL_RGB8,
	RGB8_SNORM = GL_RGB8_SNORM,
	RGB10 = GL_RGB10,
	RGB12 = GL_RGB12,
	RGB16_SNORM = GL_RGB16_SNORM,
	RGBA2 = GL_RGBA2,
	RGBA4 = GL_RGBA4,
	RGB5_A1 = GL_RGB5_A1,
	RGBA8 = GL_RGBA8,
	RGBA8_SNORM = GL_RGBA8_SNORM,
	RGB10_A2 = GL_RGB10_A2,
	RGB10_A2UI = GL_RGB10_A2UI,
	RGBA12 = GL_RGBA12,
	RGBA16 = GL_RGBA16,
	SRGB8 = GL_SRGB8,
	SRGB8_ALPHA8 = GL_SRGB8_ALPHA8,
	R16F = GL_R16F,
	RG16F = GL_RG16F,
	RGB16F = GL_RGB16F,
	RGBA16F = GL_RGBA16F,
	R32F = GL_R32F,
	RG32F = GL_RG32F,
	RGB32F = GL_RGB32F,
	RGBA32F = GL_RGBA32F,
	R11F_G11F_B10F = GL_R11F_G11F_B10F,
	RGB9_E5 = GL_RGB9_E5,
	R8I = GL_R8I,
	R8UI = GL_R8UI,
	R16I = GL_R16I,
	R16UI = GL_R16UI,
	R32I = GL_R32I,
	R32UI = GL_R32UI,
	RG8I = GL_RG8I,
	RG8UI = GL_RG8UI,
	RG16I = GL_RG16I,
	RG16UI = GL_RG16UI,
	RG32I = GL_RG32I,
	RG32UI = GL_RG32UI,
	RGB8I = GL_RGB8I,
	RGB8UI = GL_RGB8UI,
	RGB16I = GL_RGB16I,
	RGB16UI = GL_RGB16UI,
	RGB32I = GL_RGB32I,
	RGB32UI = GL_RGB32UI,
	RGBA8I = GL_RGBA8I,
	RGBA8UI = GL_RGBA8UI,
	RGBA16I = GL_RGBA16I,
	RGBA16UI = GL_RGBA16UI,
	RGBA32I = GL_RGBA32I,
	RGBA32UI = GL_RGBA32UI,

	//compressed
	COMPRESSED_RED = GL_COMPRESSED_RED,
	COMPRESSED_RG = GL_COMPRESSED_RG,
	COMPRESSED_RGB = GL_COMPRESSED_RGB,
	COMPRESSED_RGBA = GL_COMPRESSED_RGBA,
	COMPRESSED_SRGB = GL_COMPRESSED_SRGB,
	COMPRESSED_SRGB_ALPHA = GL_COMPRESSED_SRGB_ALPHA,
	COMPRESSED_RED_RGTC1 = GL_COMPRESSED_RED_RGTC1,
	COMPRESSED_SIGNED_RED_RGTC1 = GL_COMPRESSED_SIGNED_RED_RGTC1,
	COMPRESSED_RG_RGTC2 = GL_COMPRESSED_RG_RGTC2,
	COMPRESSED_SIGNED_RG_RGTC2 = GL_COMPRESSED_SIGNED_RG_RGTC2,
	COMPRESSED_RGBA_BPTC_UNORM = GL_COMPRESSED_RGBA_BPTC_UNORM,
	COMPRESSED_SRGB_ALPHA_BPTC_UNORM = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
	COMPRESSED_RGB_BPTC_SIGNED_FLOAT = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
	COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
};

struct GLTypeTable {
	GLenum upload_type;
	GPUFormat gpu_format[5];
	GLenum upload_format[5];
};

template<typename T> constexpr GLTypeTable gl_type_table = { 0,{ NONE, NONE, NONE, NONE, NONE }, { 0,0,0,0,0 } };
template<> constexpr GLTypeTable gl_type_table<f32> = { GL_FLOAT,						{ NONE, R32F	, RG32F	, RGB32F	, RGBA32F	}, { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA } };
template<> constexpr GLTypeTable gl_type_table<i32> = { GL_INT,							{ NONE, R32I	, RG32I	, RGB32I	, RGBA32I	}, { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA } };
template<> constexpr GLTypeTable gl_type_table<i16> = { GL_SHORT,						{ NONE, R16I	, RG16I	, RGB16I	, RGBA16I	}, { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA } };
template<> constexpr GLTypeTable gl_type_table<i8 > = { GL_BYTE,						{ NONE, R8I		, RG8I	, RGB8I		, RGBA8I	}, { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA } };
//* wtf https://community.khronos.org/t/invalid-operation-when-i-try-to-load-a-gl-r8ui-texture/75321
template<> constexpr GLTypeTable gl_type_table<u32> = { GL_UNSIGNED_INT,		{ NONE, R32UI	, RG32UI, RGB32UI	, RGBA32UI}, { 0, GL_RED_INTEGER, GL_RG, GL_RGB, GL_RGBA } };
template<> constexpr GLTypeTable gl_type_table<u16> = { GL_UNSIGNED_SHORT,	{ NONE, R16UI	, RG16UI, RGB16UI	, RGBA16UI}, { 0, GL_RED_INTEGER, GL_RG, GL_RGB, GL_RGBA } };
template<> constexpr GLTypeTable gl_type_table<u8 > = { GL_UNSIGNED_BYTE,		{ NONE, R8UI	, RG8UI	, RGB8UI	, RGBA8UI	}, { 0, GL_RED_INTEGER, GL_RG, GL_RGB, GL_RGBA } };

u64 index_size(GLenum index_type) {
	switch (index_type) {
		case GL_BYTE:
		case GL_UNSIGNED_BYTE: return 1;
		case GL_SHORT:
		case GL_UNSIGNED_SHORT: return 2;
		case GL_INT:
		case GL_UNSIGNED_INT: return 4;
		default: return 0;
	}
}

struct SrcFormat {
	GLenum type;
	GLenum channels;
	u32 channel_count;
	u32 channel_size;
};

template<typename P> constexpr SrcFormat Format = { 0, 0, 0, 0 };
template<typename T, i32 I> constexpr SrcFormat Format<glm::vec<I, T>> = {
	.type = gl_type_table<T>.upload_type,
	.channels = gl_type_table<T>.upload_format[I],
	.channel_count = I,
	.channel_size = sizeof(T)
};

template<> constexpr SrcFormat Format<f32> = Format<v1f32>;
template<> constexpr SrcFormat Format<i32> = Format<v1i32>;
template<> constexpr SrcFormat Format<u32> = Format<v1u32>;
template<> constexpr SrcFormat Format<i16> = Format<v1i16>;
template<> constexpr SrcFormat Format<u16> = Format<v1u16>;
template<> constexpr SrcFormat Format<i8> = Format<v1i8>;
template<> constexpr SrcFormat Format<u8> = Format<v1u8>;

template<typename T> constexpr SrcFormat Formats[] = {
	{},
	Format<glm::vec<1, T>>,
	Format<glm::vec<2, T>>,
	Format<glm::vec<3, T>>,
	Format<glm::vec<4, T>>
};

template<typename T> constexpr SrcFormat image_format_of(Array<T>) {
	return Format<std::remove_const_t<T>>;
}

#endif
