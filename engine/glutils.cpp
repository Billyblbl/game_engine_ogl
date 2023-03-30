#ifndef GGLUTILS
# define GGLUTILS

#include <blblstd.hpp>
#include <cstdio>
#include <optional>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#define serialise_macro(d) lstr(#d)

const string GLtoString(GLenum value) {
	switch (value) {
	case GL_FRAGMENT_SHADER: return serialise_macro(GL_FRAGMENT_SHADER);
	case GL_VERTEX_SHADER: return serialise_macro(GL_VERTEX_SHADER);
	case GL_DEPTH_BUFFER_BIT: return serialise_macro(GL_DEPTH_BUFFER_BIT);
	case GL_STENCIL_BUFFER_BIT: return serialise_macro(GL_STENCIL_BUFFER_BIT);
	case GL_COLOR_BUFFER_BIT: return serialise_macro(GL_COLOR_BUFFER_BIT);
	case GL_POINTS: return serialise_macro(GL_POINTS);
	case GL_LINES: return serialise_macro(GL_LINES);
	case GL_LINE_LOOP: return serialise_macro(GL_LINE_LOOP);
	case GL_LINE_STRIP: return serialise_macro(GL_LINE_STRIP);
	case GL_TRIANGLES: return serialise_macro(GL_TRIANGLES);
	case GL_TRIANGLE_STRIP: return serialise_macro(GL_TRIANGLE_STRIP);
	case GL_TRIANGLE_FAN: return serialise_macro(GL_TRIANGLE_FAN);
	case GL_QUADS: return serialise_macro(GL_QUADS);
	case GL_NEVER: return serialise_macro(GL_NEVER);
	case GL_LESS: return serialise_macro(GL_LESS);
	case GL_EQUAL: return serialise_macro(GL_EQUAL);
	case GL_LEQUAL: return serialise_macro(GL_LEQUAL);
	case GL_GREATER: return serialise_macro(GL_GREATER);
	case GL_NOTEQUAL: return serialise_macro(GL_NOTEQUAL);
	case GL_GEQUAL: return serialise_macro(GL_GEQUAL);
	case GL_ALWAYS: return serialise_macro(GL_ALWAYS);
	case GL_SRC_COLOR: return serialise_macro(GL_SRC_COLOR);
	case GL_ONE_MINUS_SRC_COLOR: return serialise_macro(GL_ONE_MINUS_SRC_COLOR);
	case GL_SRC_ALPHA: return serialise_macro(GL_SRC_ALPHA);
	case GL_ONE_MINUS_SRC_ALPHA: return serialise_macro(GL_ONE_MINUS_SRC_ALPHA);
	case GL_DST_ALPHA: return serialise_macro(GL_DST_ALPHA);
	case GL_ONE_MINUS_DST_ALPHA: return serialise_macro(GL_ONE_MINUS_DST_ALPHA);
	case GL_DST_COLOR: return serialise_macro(GL_DST_COLOR);
	case GL_ONE_MINUS_DST_COLOR: return serialise_macro(GL_ONE_MINUS_DST_COLOR);
	case GL_SRC_ALPHA_SATURATE: return serialise_macro(GL_SRC_ALPHA_SATURATE);
		// case GL_NONE: return serialiseDefine(GL_NONE);
		// case GL_FRONT_LEFT: return serialiseDefine(GL_FRONT_LEFT);
	case GL_FRONT_RIGHT: return serialise_macro(GL_FRONT_RIGHT);
	case GL_BACK_LEFT: return serialise_macro(GL_BACK_LEFT);
	case GL_BACK_RIGHT: return serialise_macro(GL_BACK_RIGHT);
	case GL_FRONT: return serialise_macro(GL_FRONT);
	case GL_BACK: return serialise_macro(GL_BACK);
	case GL_LEFT: return serialise_macro(GL_LEFT);
	case GL_RIGHT: return serialise_macro(GL_RIGHT);
	case GL_FRONT_AND_BACK: return serialise_macro(GL_FRONT_AND_BACK);
		// case GL_NO_ERROR: return serialiseDefine(GL_NO_ERROR);
	case GL_INVALID_ENUM: return serialise_macro(GL_INVALID_ENUM);
	case GL_INVALID_VALUE: return serialise_macro(GL_INVALID_VALUE);
	case GL_INVALID_OPERATION: return serialise_macro(GL_INVALID_OPERATION);
	case GL_OUT_OF_MEMORY: return serialise_macro(GL_OUT_OF_MEMORY);
	case GL_CW: return serialise_macro(GL_CW);
	case GL_CCW: return serialise_macro(GL_CCW);
	case GL_POINT_SIZE: return serialise_macro(GL_POINT_SIZE);
	case GL_POINT_SIZE_RANGE: return serialise_macro(GL_POINT_SIZE_RANGE);
	case GL_POINT_SIZE_GRANULARITY: return serialise_macro(GL_POINT_SIZE_GRANULARITY);
	case GL_LINE_SMOOTH: return serialise_macro(GL_LINE_SMOOTH);
	case GL_LINE_WIDTH: return serialise_macro(GL_LINE_WIDTH);
	case GL_LINE_WIDTH_RANGE: return serialise_macro(GL_LINE_WIDTH_RANGE);
	case GL_LINE_WIDTH_GRANULARITY: return serialise_macro(GL_LINE_WIDTH_GRANULARITY);
	case GL_POLYGON_MODE: return serialise_macro(GL_POLYGON_MODE);
	case GL_POLYGON_SMOOTH: return serialise_macro(GL_POLYGON_SMOOTH);
	case GL_CULL_FACE: return serialise_macro(GL_CULL_FACE);
	case GL_CULL_FACE_MODE: return serialise_macro(GL_CULL_FACE_MODE);
	case GL_FRONT_FACE: return serialise_macro(GL_FRONT_FACE);
	case GL_DEPTH_RANGE: return serialise_macro(GL_DEPTH_RANGE);
	case GL_DEPTH_TEST: return serialise_macro(GL_DEPTH_TEST);
	case GL_DEPTH_WRITEMASK: return serialise_macro(GL_DEPTH_WRITEMASK);
	case GL_DEPTH_CLEAR_VALUE: return serialise_macro(GL_DEPTH_CLEAR_VALUE);
	case GL_DEPTH_FUNC: return serialise_macro(GL_DEPTH_FUNC);
	case GL_STENCIL_TEST: return serialise_macro(GL_STENCIL_TEST);
	case GL_STENCIL_CLEAR_VALUE: return serialise_macro(GL_STENCIL_CLEAR_VALUE);
	case GL_STENCIL_FUNC: return serialise_macro(GL_STENCIL_FUNC);
	case GL_STENCIL_VALUE_MASK: return serialise_macro(GL_STENCIL_VALUE_MASK);
	case GL_STENCIL_FAIL: return serialise_macro(GL_STENCIL_FAIL);
	case GL_STENCIL_PASS_DEPTH_FAIL: return serialise_macro(GL_STENCIL_PASS_DEPTH_FAIL);
	case GL_STENCIL_PASS_DEPTH_PASS: return serialise_macro(GL_STENCIL_PASS_DEPTH_PASS);
	case GL_STENCIL_REF: return serialise_macro(GL_STENCIL_REF);
	case GL_STENCIL_WRITEMASK: return serialise_macro(GL_STENCIL_WRITEMASK);
	case GL_VIEWPORT: return serialise_macro(GL_VIEWPORT);
	case GL_DITHER: return serialise_macro(GL_DITHER);
	case GL_BLEND_DST: return serialise_macro(GL_BLEND_DST);
	case GL_BLEND_SRC: return serialise_macro(GL_BLEND_SRC);
	case GL_BLEND: return serialise_macro(GL_BLEND);
	case GL_LOGIC_OP_MODE: return serialise_macro(GL_LOGIC_OP_MODE);
	case GL_DRAW_BUFFER: return serialise_macro(GL_DRAW_BUFFER);
	case GL_READ_BUFFER: return serialise_macro(GL_READ_BUFFER);
	case GL_SCISSOR_BOX: return serialise_macro(GL_SCISSOR_BOX);
	case GL_SCISSOR_TEST: return serialise_macro(GL_SCISSOR_TEST);
	case GL_COLOR_CLEAR_VALUE: return serialise_macro(GL_COLOR_CLEAR_VALUE);
	case GL_COLOR_WRITEMASK: return serialise_macro(GL_COLOR_WRITEMASK);
	case GL_DOUBLEBUFFER: return serialise_macro(GL_DOUBLEBUFFER);
	case GL_STEREO: return serialise_macro(GL_STEREO);
	case GL_LINE_SMOOTH_HINT: return serialise_macro(GL_LINE_SMOOTH_HINT);
	case GL_POLYGON_SMOOTH_HINT: return serialise_macro(GL_POLYGON_SMOOTH_HINT);
	case GL_UNPACK_SWAP_BYTES: return serialise_macro(GL_UNPACK_SWAP_BYTES);
	case GL_UNPACK_LSB_FIRST: return serialise_macro(GL_UNPACK_LSB_FIRST);
	case GL_UNPACK_ROW_LENGTH: return serialise_macro(GL_UNPACK_ROW_LENGTH);
	case GL_UNPACK_SKIP_ROWS: return serialise_macro(GL_UNPACK_SKIP_ROWS);
	case GL_UNPACK_SKIP_PIXELS: return serialise_macro(GL_UNPACK_SKIP_PIXELS);
	case GL_UNPACK_ALIGNMENT: return serialise_macro(GL_UNPACK_ALIGNMENT);
	case GL_PACK_SWAP_BYTES: return serialise_macro(GL_PACK_SWAP_BYTES);
	case GL_PACK_LSB_FIRST: return serialise_macro(GL_PACK_LSB_FIRST);
	case GL_PACK_ROW_LENGTH: return serialise_macro(GL_PACK_ROW_LENGTH);
	case GL_PACK_SKIP_ROWS: return serialise_macro(GL_PACK_SKIP_ROWS);
	case GL_PACK_SKIP_PIXELS: return serialise_macro(GL_PACK_SKIP_PIXELS);
	case GL_PACK_ALIGNMENT: return serialise_macro(GL_PACK_ALIGNMENT);
	case GL_MAX_TEXTURE_SIZE: return serialise_macro(GL_MAX_TEXTURE_SIZE);
	case GL_MAX_VIEWPORT_DIMS: return serialise_macro(GL_MAX_VIEWPORT_DIMS);
	case GL_SUBPIXEL_BITS: return serialise_macro(GL_SUBPIXEL_BITS);
	case GL_TEXTURE_1D: return serialise_macro(GL_TEXTURE_1D);
	case GL_TEXTURE_2D: return serialise_macro(GL_TEXTURE_2D);
	case GL_TEXTURE_WIDTH: return serialise_macro(GL_TEXTURE_WIDTH);
	case GL_TEXTURE_HEIGHT: return serialise_macro(GL_TEXTURE_HEIGHT);
	case GL_TEXTURE_BORDER_COLOR: return serialise_macro(GL_TEXTURE_BORDER_COLOR);
	case GL_DONT_CARE: return serialise_macro(GL_DONT_CARE);
	case GL_FASTEST: return serialise_macro(GL_FASTEST);
	case GL_NICEST: return serialise_macro(GL_NICEST);
	case GL_BYTE: return serialise_macro(GL_BYTE);
	case GL_UNSIGNED_BYTE: return serialise_macro(GL_UNSIGNED_BYTE);
	case GL_SHORT: return serialise_macro(GL_SHORT);
	case GL_UNSIGNED_SHORT: return serialise_macro(GL_UNSIGNED_SHORT);
	case GL_INT: return serialise_macro(GL_INT);
	case GL_UNSIGNED_INT: return serialise_macro(GL_UNSIGNED_INT);
	case GL_FLOAT: return serialise_macro(GL_FLOAT);
	case GL_STACK_OVERFLOW: return serialise_macro(GL_STACK_OVERFLOW);
	case GL_STACK_UNDERFLOW: return serialise_macro(GL_STACK_UNDERFLOW);
	case GL_CLEAR: return serialise_macro(GL_CLEAR);
	case GL_AND: return serialise_macro(GL_AND);
	case GL_AND_REVERSE: return serialise_macro(GL_AND_REVERSE);
	case GL_COPY: return serialise_macro(GL_COPY);
	case GL_AND_INVERTED: return serialise_macro(GL_AND_INVERTED);
	case GL_NOOP: return serialise_macro(GL_NOOP);
	case GL_XOR: return serialise_macro(GL_XOR);
	case GL_OR: return serialise_macro(GL_OR);
	case GL_NOR: return serialise_macro(GL_NOR);
	case GL_EQUIV: return serialise_macro(GL_EQUIV);
	case GL_INVERT: return serialise_macro(GL_INVERT);
	case GL_OR_REVERSE: return serialise_macro(GL_OR_REVERSE);
	case GL_COPY_INVERTED: return serialise_macro(GL_COPY_INVERTED);
	case GL_OR_INVERTED: return serialise_macro(GL_OR_INVERTED);
	case GL_NAND: return serialise_macro(GL_NAND);
	case GL_SET: return serialise_macro(GL_SET);
	case GL_TEXTURE: return serialise_macro(GL_TEXTURE);
	case GL_COLOR: return serialise_macro(GL_COLOR);
	case GL_DEPTH: return serialise_macro(GL_DEPTH);
	case GL_STENCIL: return serialise_macro(GL_STENCIL);
	case GL_STENCIL_INDEX: return serialise_macro(GL_STENCIL_INDEX);
	case GL_DEPTH_COMPONENT: return serialise_macro(GL_DEPTH_COMPONENT);
	case GL_RED: return serialise_macro(GL_RED);
	case GL_GREEN: return serialise_macro(GL_GREEN);
	case GL_BLUE: return serialise_macro(GL_BLUE);
	case GL_ALPHA: return serialise_macro(GL_ALPHA);
	case GL_RGB: return serialise_macro(GL_RGB);
	case GL_RGBA: return serialise_macro(GL_RGBA);
	case GL_POINT: return serialise_macro(GL_POINT);
	case GL_LINE: return serialise_macro(GL_LINE);
	case GL_FILL: return serialise_macro(GL_FILL);
	case GL_KEEP: return serialise_macro(GL_KEEP);
	case GL_REPLACE: return serialise_macro(GL_REPLACE);
	case GL_INCR: return serialise_macro(GL_INCR);
	case GL_DECR: return serialise_macro(GL_DECR);
	case GL_VENDOR: return serialise_macro(GL_VENDOR);
	case GL_RENDERER: return serialise_macro(GL_RENDERER);
	case GL_VERSION: return serialise_macro(GL_VERSION);
	case GL_EXTENSIONS: return serialise_macro(GL_EXTENSIONS);
	case GL_NEAREST: return serialise_macro(GL_NEAREST);
	case GL_LINEAR: return serialise_macro(GL_LINEAR);
	case GL_NEAREST_MIPMAP_NEAREST: return serialise_macro(GL_NEAREST_MIPMAP_NEAREST);
	case GL_LINEAR_MIPMAP_NEAREST: return serialise_macro(GL_LINEAR_MIPMAP_NEAREST);
	case GL_NEAREST_MIPMAP_LINEAR: return serialise_macro(GL_NEAREST_MIPMAP_LINEAR);
	case GL_LINEAR_MIPMAP_LINEAR: return serialise_macro(GL_LINEAR_MIPMAP_LINEAR);
	case GL_TEXTURE_MAG_FILTER: return serialise_macro(GL_TEXTURE_MAG_FILTER);
	case GL_TEXTURE_MIN_FILTER: return serialise_macro(GL_TEXTURE_MIN_FILTER);
	case GL_TEXTURE_WRAP_S: return serialise_macro(GL_TEXTURE_WRAP_S);
	case GL_TEXTURE_WRAP_T: return serialise_macro(GL_TEXTURE_WRAP_T);
	case GL_REPEAT: return serialise_macro(GL_REPEAT);
	case GL_DEBUG_SOURCE_API: return serialise_macro(GL_DEBUG_SOURCE_API);
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return serialise_macro(GL_DEBUG_SOURCE_WINDOW_SYSTEM);
	case GL_DEBUG_SOURCE_SHADER_COMPILER: return serialise_macro(GL_DEBUG_SOURCE_SHADER_COMPILER);
	case GL_DEBUG_SOURCE_THIRD_PARTY: return serialise_macro(GL_DEBUG_SOURCE_THIRD_PARTY);
	case GL_DEBUG_SOURCE_APPLICATION: return serialise_macro(GL_DEBUG_SOURCE_APPLICATION);
	case GL_DEBUG_SOURCE_OTHER: return serialise_macro(GL_DEBUG_SOURCE_OTHER);
	case GL_DEBUG_TYPE_ERROR: return serialise_macro(GL_DEBUG_TYPE_ERROR);

	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return serialise_macro(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR);
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return serialise_macro(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR);
	case GL_DEBUG_TYPE_PORTABILITY: return serialise_macro(GL_DEBUG_TYPE_PORTABILITY);
	case GL_DEBUG_TYPE_PERFORMANCE: return serialise_macro(GL_DEBUG_TYPE_PERFORMANCE);
	case GL_DEBUG_TYPE_MARKER: return serialise_macro(GL_DEBUG_TYPE_MARKER);
	case GL_DEBUG_TYPE_PUSH_GROUP: return serialise_macro(GL_DEBUG_TYPE_PUSH_GROUP);
	case GL_DEBUG_TYPE_POP_GROUP: return serialise_macro(GL_DEBUG_TYPE_POP_GROUP);
	case GL_DEBUG_TYPE_OTHER: return serialise_macro(GL_DEBUG_TYPE_OTHER);

	case GL_DEBUG_SEVERITY_HIGH: return serialise_macro(GL_DEBUG_SEVERITY_HIGH);
	case GL_DEBUG_SEVERITY_MEDIUM: return serialise_macro(GL_DEBUG_SEVERITY_MEDIUM);
	case GL_DEBUG_SEVERITY_LOW: return serialise_macro(GL_DEBUG_SEVERITY_LOW);
	case GL_DEBUG_SEVERITY_NOTIFICATION: return serialise_macro(GL_DEBUG_SEVERITY_NOTIFICATION);

	default: return lstr("Unknown OpenGL enum");
	}
}

void CheckGLError(string expression, string file_name, u32 line_number) {
	for (auto err = glGetError(); err != GL_NO_ERROR; err = glGetError()) {
		std::printf("Error in file %s:%d, when executing %s : %x %s\n", file_name.data(), line_number, expression.data(), err, GLtoString(err));
	}
}

#define DEBUG_GL true
// #define DEBUG_GL false

#if DEBUG_GL
#define GL_GUARD(x) [&]() -> auto { defer {CheckGLError(#x, __FILE__, __LINE__);}; return x;}()
#else
#define GL_GUARD(x) x
#endif

#define CGL_BUFFER_MAPPED (GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT)

static void flush_mapped_buffer(GLuint buffer, i64range range) {
	GL_GUARD(glFlushMappedNamedBufferRange(buffer, range.b.value, range.e.value));
}

//TODO chose what to do about optional types
static GLuint create_buffer(GLsizeiptr size, Array<byte>* mapping = null, std::optional<Array<byte>> initial_values = std::nullopt) {
	GLuint id;
	GL_GUARD(glCreateBuffers(1, &id));
	auto initial_ptr = initial_values ? initial_values.value().data() : null;
	GL_GUARD(glNamedBufferStorage(id, size, initial_ptr, (mapping == null) ? 0 : CGL_BUFFER_MAPPED));
	if (mapping != null) {
		auto ptr = GL_GUARD(glMapNamedBufferRange(id, 0, size, CGL_BUFFER_MAPPED | GL_MAP_FLUSH_EXPLICIT_BIT));
		*mapping = Buffer((byte*)ptr, size);
	}
	return id;
}

template <typename T> inline static GLuint create_buffer_array(Array<T> buffer, Array<T>* mapping = null) {
	Array<byte> data;
	auto id = create_buffer(buffer.size_bytes(), mapping == null ? null : &data, cast<byte>(buffer));
	if (mapping != null) *mapping = cast<T>(data);
	return id;
}


template <typename T> inline static GLuint create_buffer_single(const T& buffer, T** mapping = null) {
	Array<byte> data;
	auto id = create_buffer(sizeof(T), mapping == null ? null : &data, cast<byte>(Array<const T>(&buffer, 1)));
	if (mapping != null) *mapping = cast<T>(data).data();
	return id;
}

void unmap(GLuint buffer, GLsizeiptr size) {
	flush_mapped_buffer(buffer, {0, size});
	GL_GUARD(glUnmapNamedBuffer(buffer));
}

void delete_buffer(GLuint buffer) {
	GL_GUARD(glDeleteBuffers(1, &buffer));
}

struct GLFormat {
	GLenum element;
	GLenum vec[5];
	Array<const GLenum> pixel;
};

constexpr GLenum ZeroedOut[] = { 0,0,0,0,0 };
constexpr GLenum FloatsPixelFormats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };
constexpr GLenum IntegerPixelFormats[] = { 0, GL_RED_INTEGER, GL_RG_INTEGER, GL_RGB_INTEGER, GL_RGBA_INTEGER };

template<typename T> constexpr GLFormat Format = { 0,{0,0,0,0,0},ZeroedOut };
template<> constexpr GLFormat Format<glm::f32> = { GL_FLOAT, { 0, GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F }, FloatsPixelFormats };
template<> constexpr GLFormat Format<glm::i32> = { GL_INT, { 0, GL_R32I, GL_RG32I, GL_RGB32I, GL_RGBA32I }, IntegerPixelFormats };
template<> constexpr GLFormat Format<glm::u32> = { GL_UNSIGNED_INT, { 0, GL_R32UI, GL_RG32UI, GL_RGB32UI, GL_RGBA32UI }, IntegerPixelFormats };
template<> constexpr GLFormat Format<glm::i16> = { GL_SHORT, { 0, GL_R16I, GL_RG16I, GL_RGB16I, GL_RGBA16I }, IntegerPixelFormats };
template<> constexpr GLFormat Format<glm::u16> = { GL_UNSIGNED_SHORT, { 0, GL_R16UI, GL_RG16UI, GL_RGB16UI, GL_RGBA16UI }, IntegerPixelFormats };
template<> constexpr GLFormat Format<glm::i8> = { GL_BYTE, { 0, GL_R8I, GL_RG8I, GL_RGB8I, GL_RGBA8I }, IntegerPixelFormats };
template<> constexpr GLFormat Format<glm::u8> = { GL_UNSIGNED_BYTE, { 0, GL_R8UI, GL_RG8UI, GL_RGB8UI, GL_RGBA8UI }, IntegerPixelFormats };

// constexpr GLFormat FormatUntyped = Format<void>;
#endif
