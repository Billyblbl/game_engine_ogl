#ifndef GGLUTILS
# define GGLUTILS

#include <string_view>
#include <span>
#include <cstdio>
#include <optional>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#define serialiseDefine(d) #d

const char* GLtoString(GLenum value) {
	switch (value) {
	case GL_DEPTH_BUFFER_BIT: return serialiseDefine(GL_DEPTH_BUFFER_BIT);
	case GL_STENCIL_BUFFER_BIT: return serialiseDefine(GL_STENCIL_BUFFER_BIT);
	case GL_COLOR_BUFFER_BIT: return serialiseDefine(GL_COLOR_BUFFER_BIT);
	case GL_POINTS: return serialiseDefine(GL_POINTS);
	case GL_LINES: return serialiseDefine(GL_LINES);
	case GL_LINE_LOOP: return serialiseDefine(GL_LINE_LOOP);
	case GL_LINE_STRIP: return serialiseDefine(GL_LINE_STRIP);
	case GL_TRIANGLES: return serialiseDefine(GL_TRIANGLES);
	case GL_TRIANGLE_STRIP: return serialiseDefine(GL_TRIANGLE_STRIP);
	case GL_TRIANGLE_FAN: return serialiseDefine(GL_TRIANGLE_FAN);
	case GL_QUADS: return serialiseDefine(GL_QUADS);
	case GL_NEVER: return serialiseDefine(GL_NEVER);
	case GL_LESS: return serialiseDefine(GL_LESS);
	case GL_EQUAL: return serialiseDefine(GL_EQUAL);
	case GL_LEQUAL: return serialiseDefine(GL_LEQUAL);
	case GL_GREATER: return serialiseDefine(GL_GREATER);
	case GL_NOTEQUAL: return serialiseDefine(GL_NOTEQUAL);
	case GL_GEQUAL: return serialiseDefine(GL_GEQUAL);
	case GL_ALWAYS: return serialiseDefine(GL_ALWAYS);
	case GL_SRC_COLOR: return serialiseDefine(GL_SRC_COLOR);
	case GL_ONE_MINUS_SRC_COLOR: return serialiseDefine(GL_ONE_MINUS_SRC_COLOR);
	case GL_SRC_ALPHA: return serialiseDefine(GL_SRC_ALPHA);
	case GL_ONE_MINUS_SRC_ALPHA: return serialiseDefine(GL_ONE_MINUS_SRC_ALPHA);
	case GL_DST_ALPHA: return serialiseDefine(GL_DST_ALPHA);
	case GL_ONE_MINUS_DST_ALPHA: return serialiseDefine(GL_ONE_MINUS_DST_ALPHA);
	case GL_DST_COLOR: return serialiseDefine(GL_DST_COLOR);
	case GL_ONE_MINUS_DST_COLOR: return serialiseDefine(GL_ONE_MINUS_DST_COLOR);
	case GL_SRC_ALPHA_SATURATE: return serialiseDefine(GL_SRC_ALPHA_SATURATE);
		// case GL_NONE: return serialiseDefine(GL_NONE);
		// case GL_FRONT_LEFT: return serialiseDefine(GL_FRONT_LEFT);
	case GL_FRONT_RIGHT: return serialiseDefine(GL_FRONT_RIGHT);
	case GL_BACK_LEFT: return serialiseDefine(GL_BACK_LEFT);
	case GL_BACK_RIGHT: return serialiseDefine(GL_BACK_RIGHT);
	case GL_FRONT: return serialiseDefine(GL_FRONT);
	case GL_BACK: return serialiseDefine(GL_BACK);
	case GL_LEFT: return serialiseDefine(GL_LEFT);
	case GL_RIGHT: return serialiseDefine(GL_RIGHT);
	case GL_FRONT_AND_BACK: return serialiseDefine(GL_FRONT_AND_BACK);
		// case GL_NO_ERROR: return serialiseDefine(GL_NO_ERROR);
	case GL_INVALID_ENUM: return serialiseDefine(GL_INVALID_ENUM);
	case GL_INVALID_VALUE: return serialiseDefine(GL_INVALID_VALUE);
	case GL_INVALID_OPERATION: return serialiseDefine(GL_INVALID_OPERATION);
	case GL_OUT_OF_MEMORY: return serialiseDefine(GL_OUT_OF_MEMORY);
	case GL_CW: return serialiseDefine(GL_CW);
	case GL_CCW: return serialiseDefine(GL_CCW);
	case GL_POINT_SIZE: return serialiseDefine(GL_POINT_SIZE);
	case GL_POINT_SIZE_RANGE: return serialiseDefine(GL_POINT_SIZE_RANGE);
	case GL_POINT_SIZE_GRANULARITY: return serialiseDefine(GL_POINT_SIZE_GRANULARITY);
	case GL_LINE_SMOOTH: return serialiseDefine(GL_LINE_SMOOTH);
	case GL_LINE_WIDTH: return serialiseDefine(GL_LINE_WIDTH);
	case GL_LINE_WIDTH_RANGE: return serialiseDefine(GL_LINE_WIDTH_RANGE);
	case GL_LINE_WIDTH_GRANULARITY: return serialiseDefine(GL_LINE_WIDTH_GRANULARITY);
	case GL_POLYGON_MODE: return serialiseDefine(GL_POLYGON_MODE);
	case GL_POLYGON_SMOOTH: return serialiseDefine(GL_POLYGON_SMOOTH);
	case GL_CULL_FACE: return serialiseDefine(GL_CULL_FACE);
	case GL_CULL_FACE_MODE: return serialiseDefine(GL_CULL_FACE_MODE);
	case GL_FRONT_FACE: return serialiseDefine(GL_FRONT_FACE);
	case GL_DEPTH_RANGE: return serialiseDefine(GL_DEPTH_RANGE);
	case GL_DEPTH_TEST: return serialiseDefine(GL_DEPTH_TEST);
	case GL_DEPTH_WRITEMASK: return serialiseDefine(GL_DEPTH_WRITEMASK);
	case GL_DEPTH_CLEAR_VALUE: return serialiseDefine(GL_DEPTH_CLEAR_VALUE);
	case GL_DEPTH_FUNC: return serialiseDefine(GL_DEPTH_FUNC);
	case GL_STENCIL_TEST: return serialiseDefine(GL_STENCIL_TEST);
	case GL_STENCIL_CLEAR_VALUE: return serialiseDefine(GL_STENCIL_CLEAR_VALUE);
	case GL_STENCIL_FUNC: return serialiseDefine(GL_STENCIL_FUNC);
	case GL_STENCIL_VALUE_MASK: return serialiseDefine(GL_STENCIL_VALUE_MASK);
	case GL_STENCIL_FAIL: return serialiseDefine(GL_STENCIL_FAIL);
	case GL_STENCIL_PASS_DEPTH_FAIL: return serialiseDefine(GL_STENCIL_PASS_DEPTH_FAIL);
	case GL_STENCIL_PASS_DEPTH_PASS: return serialiseDefine(GL_STENCIL_PASS_DEPTH_PASS);
	case GL_STENCIL_REF: return serialiseDefine(GL_STENCIL_REF);
	case GL_STENCIL_WRITEMASK: return serialiseDefine(GL_STENCIL_WRITEMASK);
	case GL_VIEWPORT: return serialiseDefine(GL_VIEWPORT);
	case GL_DITHER: return serialiseDefine(GL_DITHER);
	case GL_BLEND_DST: return serialiseDefine(GL_BLEND_DST);
	case GL_BLEND_SRC: return serialiseDefine(GL_BLEND_SRC);
	case GL_BLEND: return serialiseDefine(GL_BLEND);
	case GL_LOGIC_OP_MODE: return serialiseDefine(GL_LOGIC_OP_MODE);
	case GL_DRAW_BUFFER: return serialiseDefine(GL_DRAW_BUFFER);
	case GL_READ_BUFFER: return serialiseDefine(GL_READ_BUFFER);
	case GL_SCISSOR_BOX: return serialiseDefine(GL_SCISSOR_BOX);
	case GL_SCISSOR_TEST: return serialiseDefine(GL_SCISSOR_TEST);
	case GL_COLOR_CLEAR_VALUE: return serialiseDefine(GL_COLOR_CLEAR_VALUE);
	case GL_COLOR_WRITEMASK: return serialiseDefine(GL_COLOR_WRITEMASK);
	case GL_DOUBLEBUFFER: return serialiseDefine(GL_DOUBLEBUFFER);
	case GL_STEREO: return serialiseDefine(GL_STEREO);
	case GL_LINE_SMOOTH_HINT: return serialiseDefine(GL_LINE_SMOOTH_HINT);
	case GL_POLYGON_SMOOTH_HINT: return serialiseDefine(GL_POLYGON_SMOOTH_HINT);
	case GL_UNPACK_SWAP_BYTES: return serialiseDefine(GL_UNPACK_SWAP_BYTES);
	case GL_UNPACK_LSB_FIRST: return serialiseDefine(GL_UNPACK_LSB_FIRST);
	case GL_UNPACK_ROW_LENGTH: return serialiseDefine(GL_UNPACK_ROW_LENGTH);
	case GL_UNPACK_SKIP_ROWS: return serialiseDefine(GL_UNPACK_SKIP_ROWS);
	case GL_UNPACK_SKIP_PIXELS: return serialiseDefine(GL_UNPACK_SKIP_PIXELS);
	case GL_UNPACK_ALIGNMENT: return serialiseDefine(GL_UNPACK_ALIGNMENT);
	case GL_PACK_SWAP_BYTES: return serialiseDefine(GL_PACK_SWAP_BYTES);
	case GL_PACK_LSB_FIRST: return serialiseDefine(GL_PACK_LSB_FIRST);
	case GL_PACK_ROW_LENGTH: return serialiseDefine(GL_PACK_ROW_LENGTH);
	case GL_PACK_SKIP_ROWS: return serialiseDefine(GL_PACK_SKIP_ROWS);
	case GL_PACK_SKIP_PIXELS: return serialiseDefine(GL_PACK_SKIP_PIXELS);
	case GL_PACK_ALIGNMENT: return serialiseDefine(GL_PACK_ALIGNMENT);
	case GL_MAX_TEXTURE_SIZE: return serialiseDefine(GL_MAX_TEXTURE_SIZE);
	case GL_MAX_VIEWPORT_DIMS: return serialiseDefine(GL_MAX_VIEWPORT_DIMS);
	case GL_SUBPIXEL_BITS: return serialiseDefine(GL_SUBPIXEL_BITS);
	case GL_TEXTURE_1D: return serialiseDefine(GL_TEXTURE_1D);
	case GL_TEXTURE_2D: return serialiseDefine(GL_TEXTURE_2D);
	case GL_TEXTURE_WIDTH: return serialiseDefine(GL_TEXTURE_WIDTH);
	case GL_TEXTURE_HEIGHT: return serialiseDefine(GL_TEXTURE_HEIGHT);
	case GL_TEXTURE_BORDER_COLOR: return serialiseDefine(GL_TEXTURE_BORDER_COLOR);
	case GL_DONT_CARE: return serialiseDefine(GL_DONT_CARE);
	case GL_FASTEST: return serialiseDefine(GL_FASTEST);
	case GL_NICEST: return serialiseDefine(GL_NICEST);
	case GL_BYTE: return serialiseDefine(GL_BYTE);
	case GL_UNSIGNED_BYTE: return serialiseDefine(GL_UNSIGNED_BYTE);
	case GL_SHORT: return serialiseDefine(GL_SHORT);
	case GL_UNSIGNED_SHORT: return serialiseDefine(GL_UNSIGNED_SHORT);
	case GL_INT: return serialiseDefine(GL_INT);
	case GL_UNSIGNED_INT: return serialiseDefine(GL_UNSIGNED_INT);
	case GL_FLOAT: return serialiseDefine(GL_FLOAT);
	case GL_STACK_OVERFLOW: return serialiseDefine(GL_STACK_OVERFLOW);
	case GL_STACK_UNDERFLOW: return serialiseDefine(GL_STACK_UNDERFLOW);
	case GL_CLEAR: return serialiseDefine(GL_CLEAR);
	case GL_AND: return serialiseDefine(GL_AND);
	case GL_AND_REVERSE: return serialiseDefine(GL_AND_REVERSE);
	case GL_COPY: return serialiseDefine(GL_COPY);
	case GL_AND_INVERTED: return serialiseDefine(GL_AND_INVERTED);
	case GL_NOOP: return serialiseDefine(GL_NOOP);
	case GL_XOR: return serialiseDefine(GL_XOR);
	case GL_OR: return serialiseDefine(GL_OR);
	case GL_NOR: return serialiseDefine(GL_NOR);
	case GL_EQUIV: return serialiseDefine(GL_EQUIV);
	case GL_INVERT: return serialiseDefine(GL_INVERT);
	case GL_OR_REVERSE: return serialiseDefine(GL_OR_REVERSE);
	case GL_COPY_INVERTED: return serialiseDefine(GL_COPY_INVERTED);
	case GL_OR_INVERTED: return serialiseDefine(GL_OR_INVERTED);
	case GL_NAND: return serialiseDefine(GL_NAND);
	case GL_SET: return serialiseDefine(GL_SET);
	case GL_TEXTURE: return serialiseDefine(GL_TEXTURE);
	case GL_COLOR: return serialiseDefine(GL_COLOR);
	case GL_DEPTH: return serialiseDefine(GL_DEPTH);
	case GL_STENCIL: return serialiseDefine(GL_STENCIL);
	case GL_STENCIL_INDEX: return serialiseDefine(GL_STENCIL_INDEX);
	case GL_DEPTH_COMPONENT: return serialiseDefine(GL_DEPTH_COMPONENT);
	case GL_RED: return serialiseDefine(GL_RED);
	case GL_GREEN: return serialiseDefine(GL_GREEN);
	case GL_BLUE: return serialiseDefine(GL_BLUE);
	case GL_ALPHA: return serialiseDefine(GL_ALPHA);
	case GL_RGB: return serialiseDefine(GL_RGB);
	case GL_RGBA: return serialiseDefine(GL_RGBA);
	case GL_POINT: return serialiseDefine(GL_POINT);
	case GL_LINE: return serialiseDefine(GL_LINE);
	case GL_FILL: return serialiseDefine(GL_FILL);
	case GL_KEEP: return serialiseDefine(GL_KEEP);
	case GL_REPLACE: return serialiseDefine(GL_REPLACE);
	case GL_INCR: return serialiseDefine(GL_INCR);
	case GL_DECR: return serialiseDefine(GL_DECR);
	case GL_VENDOR: return serialiseDefine(GL_VENDOR);
	case GL_RENDERER: return serialiseDefine(GL_RENDERER);
	case GL_VERSION: return serialiseDefine(GL_VERSION);
	case GL_EXTENSIONS: return serialiseDefine(GL_EXTENSIONS);
	case GL_NEAREST: return serialiseDefine(GL_NEAREST);
	case GL_LINEAR: return serialiseDefine(GL_LINEAR);
	case GL_NEAREST_MIPMAP_NEAREST: return serialiseDefine(GL_NEAREST_MIPMAP_NEAREST);
	case GL_LINEAR_MIPMAP_NEAREST: return serialiseDefine(GL_LINEAR_MIPMAP_NEAREST);
	case GL_NEAREST_MIPMAP_LINEAR: return serialiseDefine(GL_NEAREST_MIPMAP_LINEAR);
	case GL_LINEAR_MIPMAP_LINEAR: return serialiseDefine(GL_LINEAR_MIPMAP_LINEAR);
	case GL_TEXTURE_MAG_FILTER: return serialiseDefine(GL_TEXTURE_MAG_FILTER);
	case GL_TEXTURE_MIN_FILTER: return serialiseDefine(GL_TEXTURE_MIN_FILTER);
	case GL_TEXTURE_WRAP_S: return serialiseDefine(GL_TEXTURE_WRAP_S);
	case GL_TEXTURE_WRAP_T: return serialiseDefine(GL_TEXTURE_WRAP_T);
	case GL_REPEAT: return serialiseDefine(GL_REPEAT);
	case GL_DEBUG_SOURCE_API: return serialiseDefine(GL_DEBUG_SOURCE_API);
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return serialiseDefine(GL_DEBUG_SOURCE_WINDOW_SYSTEM);
	case GL_DEBUG_SOURCE_SHADER_COMPILER: return serialiseDefine(GL_DEBUG_SOURCE_SHADER_COMPILER);
	case GL_DEBUG_SOURCE_THIRD_PARTY: return serialiseDefine(GL_DEBUG_SOURCE_THIRD_PARTY);
	case GL_DEBUG_SOURCE_APPLICATION: return serialiseDefine(GL_DEBUG_SOURCE_APPLICATION);
	case GL_DEBUG_SOURCE_OTHER: return serialiseDefine(GL_DEBUG_SOURCE_OTHER);
	case GL_DEBUG_TYPE_ERROR: return serialiseDefine(GL_DEBUG_TYPE_ERROR);

	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return serialiseDefine(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR);
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return serialiseDefine(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR);
	case GL_DEBUG_TYPE_PORTABILITY: return serialiseDefine(GL_DEBUG_TYPE_PORTABILITY);
	case GL_DEBUG_TYPE_PERFORMANCE: return serialiseDefine(GL_DEBUG_TYPE_PERFORMANCE);
	case GL_DEBUG_TYPE_MARKER: return serialiseDefine(GL_DEBUG_TYPE_MARKER);
	case GL_DEBUG_TYPE_PUSH_GROUP: return serialiseDefine(GL_DEBUG_TYPE_PUSH_GROUP);
	case GL_DEBUG_TYPE_POP_GROUP: return serialiseDefine(GL_DEBUG_TYPE_POP_GROUP);
	case GL_DEBUG_TYPE_OTHER: return serialiseDefine(GL_DEBUG_TYPE_OTHER);

	case GL_DEBUG_SEVERITY_HIGH: return serialiseDefine(GL_DEBUG_SEVERITY_HIGH);
	case GL_DEBUG_SEVERITY_MEDIUM: return serialiseDefine(GL_DEBUG_SEVERITY_MEDIUM);
	case GL_DEBUG_SEVERITY_LOW: return serialiseDefine(GL_DEBUG_SEVERITY_LOW);
	case GL_DEBUG_SEVERITY_NOTIFICATION: return serialiseDefine(GL_DEBUG_SEVERITY_NOTIFICATION);

	default: return "Unknown OpenGL enum";
	}
}

void CheckGLError(const std::string_view& expression, const std::string_view& fileName, const int32_t lineNumber) {
	for (auto err = glGetError(); err != GL_NO_ERROR; err = glGetError()) {
		std::printf("Error in file %s:%d, when executing %s : %x %s\n", fileName.data(), lineNumber, expression.data(), err, GLtoString(err));
	}
}

#define DEBUG_GL true
// #define DEBUG_GL false

#if DEBUG_GL
#define GL_GUARD(x) x; CheckGLError(#x, __FILE__, __LINE__)
#else
#define GL_GUARD(x) x
#endif

#define CGL_BUFFER_MAPPED (GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT)

template<typename T, typename U, size_t S> auto spanCast(std::span<U, S> original) {
	return std::span((T*)original.data(), original.size_bytes() / sizeof(T));
}

static GLuint createBuffer(GLsizeiptr size, std::span<std::byte>* mapping = nullptr, std::optional<std::span<std::byte>> initialValues = std::nullopt) {
	GLuint id;
	GL_GUARD(glCreateBuffers(1, &id));
	auto initialPtr = initialValues ? initialValues.value().data() : nullptr;
	GL_GUARD(glNamedBufferStorage(id, size, initialPtr, (mapping == nullptr) ? 0 : CGL_BUFFER_MAPPED));
	if (mapping != nullptr) {
		auto ptr = GL_GUARD(glMapNamedBufferRange(id, 0, size, CGL_BUFFER_MAPPED | GL_MAP_FLUSH_EXPLICIT_BIT));
		*mapping = std::span(reinterpret_cast<std::byte*>(ptr), size);
	}
	return id;
}

template <typename T, size_t S> static GLuint createBufferSpan(const std::span<T, S> buffer, std::span<T>* mapping = nullptr) {
	std::span<std::byte> data;
	auto id =  createBuffer(buffer.size_bytes(), mapping == nullptr ? nullptr : &data, spanCast<std::byte>(buffer));
	if (mapping != nullptr) *mapping = spanCast<T>(data);
	return id;
}


template <typename T> static GLuint createBufferSingle(const T& buffer, T** mapping = nullptr) {
	std::span<std::byte> data;
	auto id =  createBuffer(sizeof(T), mapping == nullptr ? nullptr : &data, spanCast<std::byte>(std::span(&buffer, 1)));
	if (mapping != nullptr) *mapping = spanCast<T>(data).data();
	return id;
}

struct GLFormat {
	GLenum element;
	GLenum vec[5];
	std::span<const GLenum, 5> pixel;
};

constexpr GLenum ZeroedOut[] = {0,0,0,0,0};
constexpr GLenum FloatsPixelFormats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };
constexpr GLenum IntegerPixelFormats[] = { 0, GL_RED_INTEGER, GL_RG_INTEGER, GL_RGB_INTEGER, GL_RGBA_INTEGER };

template<typename T> constexpr GLFormat Format = {0,{0,0,0,0,0},ZeroedOut};
template<> constexpr GLFormat Format<glm::f32> = { GL_FLOAT, { 0, GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F }, FloatsPixelFormats };
template<> constexpr GLFormat Format<glm::i32> = { GL_INT, { 0, GL_R32I, GL_RG32I, GL_RGB32I, GL_RGBA32I }, IntegerPixelFormats };
template<> constexpr GLFormat Format<glm::u32> = { GL_UNSIGNED_INT, { 0, GL_R32UI, GL_RG32UI, GL_RGB32UI, GL_RGBA32UI }, IntegerPixelFormats };
template<> constexpr GLFormat Format<glm::i16> = { GL_SHORT, { 0, GL_R16I, GL_RG16I, GL_RGB16I, GL_RGBA16I }, IntegerPixelFormats };
template<> constexpr GLFormat Format<glm::u16> = { GL_UNSIGNED_SHORT, { 0, GL_R16UI, GL_RG16UI, GL_RGB16UI, GL_RGBA16UI }, IntegerPixelFormats };
template<> constexpr GLFormat Format<glm::i8> = { GL_BYTE, { 0, GL_R8I, GL_RG8I, GL_RGB8I, GL_RGBA8I }, IntegerPixelFormats };
template<> constexpr GLFormat Format<glm::u8> = { GL_UNSIGNED_BYTE, { 0, GL_R8UI, GL_RG8UI, GL_RGB8UI, GL_RGBA8UI }, IntegerPixelFormats };

// constexpr GLFormat FormatUntyped = Format<void>;
#endif
