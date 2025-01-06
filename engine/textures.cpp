#ifndef GTEXTURES
# define GTEXTURES

#include <glm/glm.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <GL/glext.h>

// #include <GL/glext.h>
#include <glutils.cpp>
#include <glresource.cpp>
#include <string>
#include <math.cpp>

#include <imgui_extension.cpp>

enum SamplingFilter : GLint {
	Nearest = GL_NEAREST,
	Linear = GL_LINEAR,
	NearestMipmapNearest = GL_NEAREST_MIPMAP_NEAREST,
	LinearMipmapNearest = GL_LINEAR_MIPMAP_NEAREST,
	NearestMipmapLinear = GL_NEAREST_MIPMAP_LINEAR,
	LinearMipmapLinear = GL_LINEAR_MIPMAP_LINEAR
};

enum WrapMode : GLint {
	Clamp = GL_CLAMP_TO_EDGE,
	MirroredRepeat = GL_MIRRORED_REPEAT,
	Repeat = GL_REPEAT,
	ClampToBorder = GL_CLAMP_TO_BORDER,
	MirroredClamp = GL_MIRROR_CLAMP_TO_EDGE
};

enum DSTextureMode : GLint {
	Depth = GL_DEPTH_COMPONENT,
	Stencil = GL_STENCIL_INDEX
};

enum Comparison : GLint {
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

enum Swizzle : GLint {
	Red = GL_RED,
	Green = GL_GREEN,
	Blue = GL_BLUE,
	Alpha = GL_ALPHA,
	One = GL_ONE,
	Zero = GL_ZERO,
};

enum TexType : GLenum {
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

template<u32 D> using Area = reg_polytope<glm::vec<D, u32>>;

struct TexBuffer {
	GLuint	id = 0;
	v4u32		dimensions = v4u32(0);
	TexType type = NoType;
	GPUFormat format = {};

	static TexBuffer create(GLScope& ctx, TexType type, v4u32 dimensions, GPUFormat format = RGBA32F) {
		GLuint id;
		GL_GUARD(glCreateTextures(type, 1, &id));
		ctx.push<&GLScope::textures>(id);
		printf("Creating %s:%ux%ux%ux%u id %u\n", GLtoString(type).data(), dimensions.x, dimensions.y, dimensions.z, dimensions.w, id);
		switch (type) {
			case TX1D:		GL_GUARD(glTextureStorage1D(id, dimensions.w, format, dimensions.x)); break;
			case TX2D:
			case TX1DARR:	GL_GUARD(glTextureStorage2D(id, dimensions.w, format, dimensions.x, dimensions.y)); break;
			case TX3D:
			case TX2DARR:	GL_GUARD(glTextureStorage3D(id, dimensions.w, format, dimensions.x, dimensions.y, dimensions.z)); break;
			default: return fail_ret(GLtoString(type).data(), TexBuffer{});
		}
		return TexBuffer{
			.id = id,
			.dimensions = dimensions,
			.type = type,
			.format = format
		};
	}

	bool upload(Array<byte> source, SrcFormat format, Area<3> box, GLint mipmap = 0) {
		auto pixel_size = format.channel_count * format.channel_size;
		if (pixel_size == 0 || source.size_bytes() == 0) return false;
		else if (pixel_size % 2 == 1) {//odd size
			GL_GUARD(glPixelStorei(GL_PACK_ALIGNMENT, 1));
			GL_GUARD(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
		} else if ((pixel_size / 2) % 2 == 1) {//even * odd size
			GL_GUARD(glPixelStorei(GL_PACK_ALIGNMENT, 2));
			GL_GUARD(glPixelStorei(GL_UNPACK_ALIGNMENT, 2));
		} else {//even * even size (clamped to 8)
			GL_GUARD(glPixelStorei(GL_PACK_ALIGNMENT, min(pixel_size, 8u)));
			GL_GUARD(glPixelStorei(GL_UNPACK_ALIGNMENT, min(pixel_size, 8u)));
		}

		switch (type) {
		case TX1D:		GL_GUARD(glTextureSubImage1D(id, mipmap, box.min.x, width(box), format.channels, format.type, source.data())); break;
		case TX2D:
		case TX1DARR:	GL_GUARD(glTextureSubImage2D(id, mipmap, box.min.x, box.min.y, width(box), height(box), format.channels, format.type, source.data())); break;
		case TX3D:
		case TX2DARR:	GL_GUARD(glTextureSubImage3D(id, mipmap, box.min.x, box.min.y, box.min.z, width(box), height(box), depth(box), format.channels, format.type, source.data())); break;
		default: return fail_ret(GLtoString(type).data(), false);
		}
		return true;
	}

	template<typename T, u32 D> bool upload_as(Array<T> source, Area<D> area) {
		//TODO proper area fit check
		//TODO differentiate RGB vs sRGB texture upload https://youtu.be/MzJwiEIGj7k?t=2002
		// expect(glm::length2(area.max - area.min) <= source.size());
		return upload(cast<byte>(source), Format<T>, area);
	}

	template<typename T, u32 D> bool upload_as(Array<T> source, Area<D> area, u32 index) {
		return upload(source, slice_to_area(area, index));
	}

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

	TexBuffer& conf_border_color(v4f32 color) {
		GL_GUARD(glTextureParameterfv(id, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(color)));
		return *this;
	}

	TexBuffer& conf_border_color(v4i32 color) {
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

	TexBuffer& conf_max_sample_count(f32 max) {
		GL_GUARD(glTextureParameterf(id, GL_TEXTURE_MAX_ANISOTROPY, max));
		return *this;
	}

	TexBuffer& generate_mipmaps() {
		GL_GUARD(glGenerateTextureMipmap(id));
		return *this;
	}

	GLuint bind_unit(GLuint unit) const {
		GL_GUARD(glBindTextureUnit(unit, id));
		return id;
	}

};

template<u32 D> Area<D + 1> slice_to_area(Area<D> slice, u32 index) { return { { slice.min, index }, { slice.max, index + 1 } }; }


bool EditorWidget(const cstr label, TexBuffer& buffer) {
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		ImGui::BeginDisabled();
		ImGui::Text("id : %u", buffer.id);
		ImGui::Text("Dimensions : ( %u, %u, %u, %u )", buffer.dimensions.x, buffer.dimensions.y, buffer.dimensions.z, buffer.dimensions.w);
		ImGui::Text("Type : %s", GLtoString(buffer.type).data());
		ImGui::Text("Format : %s", GLtoString(buffer.format).data());
		ImGui::EndDisabled();
		ImGui::BeginChild("Preview", ImVec2(0, 0), ImGuiChildFlags_ResizeY); defer{ ImGui::EndChild(); };
		if (buffer.type == TX2D)
			ImGui::Image((ImTextureID)buffer.id, ImGui::fit_to_window(ImGui::from_glm(buffer.dimensions)), ImVec2(0, 1), ImVec2(1, 0));
		else
			ImGui::Text("Unimplemented preview for texture type : %s", GLtoString(buffer.type).data());
	}
	return false;
}

#endif
