#ifndef GFRAMEBUFFER
# define GFRAMEBUFFER

#include <GL/glew.h>
#include <math.cpp>
#include <glutils.cpp>
#include <textures.cpp>
#include <tuple>
#include <blblstd.hpp>

tuple<bool, string> is_complete(GLuint fbf) {
	auto status = GL_GUARD(glCheckNamedFramebufferStatus(fbf, GL_FRAMEBUFFER));
	return tuple(status == GL_FRAMEBUFFER_COMPLETE, GLtoString(status));
}

struct RenderTarget {

	struct Slot {
		GLenum id;
		GLenum type;
		GLbitfield flags;
	};
	//* default framebuffer targets
	static constexpr Slot None = { GL_NONE, 0, 0 };
	static constexpr Slot Front = { GL_FRONT, GL_COLOR, GL_COLOR_BUFFER_BIT };
	static constexpr Slot Back = { GL_BACK, GL_COLOR, GL_COLOR_BUFFER_BIT };
	static constexpr Slot Left = { GL_LEFT, GL_COLOR, GL_COLOR_BUFFER_BIT };
	static constexpr Slot Right = { GL_RIGHT, GL_COLOR, GL_COLOR_BUFFER_BIT };
	static constexpr Slot FrontAndBack = { GL_FRONT_AND_BACK, GL_COLOR, GL_COLOR_BUFFER_BIT };
	static constexpr Slot FrontLeft = { GL_FRONT_LEFT, GL_COLOR, GL_COLOR_BUFFER_BIT };
	static constexpr Slot FrontRight = { GL_FRONT_RIGHT, GL_COLOR, GL_COLOR_BUFFER_BIT };
	static constexpr Slot BackLeft = { GL_BACK_LEFT, GL_COLOR, GL_COLOR_BUFFER_BIT };
	static constexpr Slot BackRight = { GL_BACK_RIGHT, GL_COLOR, GL_COLOR_BUFFER_BIT };

	//* FBO targets
	static constexpr Slot Depth = { GL_DEPTH_ATTACHMENT, GL_DEPTH, GL_DEPTH_BUFFER_BIT };
	static constexpr Slot Stencil = { GL_STENCIL_ATTACHMENT, GL_STENCIL, GL_STENCIL_BUFFER_BIT };
	static constexpr Slot DepthStencil = { GL_DEPTH_STENCIL_ATTACHMENT, GL_DEPTH_STENCIL, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT };
	static constexpr Slot Color[] = {
		{ GL_COLOR_ATTACHMENT0, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT1, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT2, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT3, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT4, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT5, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT6, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT7, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT8, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT9, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT10, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT11, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT12, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT13, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT14, GL_COLOR, GL_COLOR_BUFFER_BIT },
		{ GL_COLOR_ATTACHMENT15, GL_COLOR, GL_COLOR_BUFFER_BIT }
	};

	struct Attachement {
		Slot slot;
		TexBuffer texture;
		GLint mip;
		GLint layer;
	};

	v2u32 dimensions;
	GLuint framebuffer;
	Array<Attachement> attachments;

	static RenderTarget create(GLScope& ctx, LiteralArray<Attachement> bindings) { return create(ctx, larray(bindings)); }
	static RenderTarget create(GLScope& ctx, Array<const Attachement> bindings) {
		GLuint id;
		GL_GUARD(glCreateFramebuffers(1, &id));
		ctx.push<&GLScope::framebuffers>(id);
		v4u32 min_dimensions = v4u32(0);

		auto attachements = map(ctx.arena, bindings,
			[&](const Attachement& attch) -> Attachement {
				min_dimensions = glm::max(min_dimensions, attch.texture.dimensions);
				switch (attch.texture.type) {
				case TX2DARR: case TX3D: glNamedFramebufferTextureLayer(id, attch.slot.id, attch.texture.id, attch.mip, attch.layer); break;
				default: glNamedFramebufferTexture(id, attch.slot.id, attch.texture.id, attch.mip);
				}
				return attch;
			}
		);

		return {
			.dimensions = min_dimensions,
			.framebuffer = id,
			.attachments = attachements
		};
	}

	static RenderTarget make_default(GLScope& ctx, v2u32 dimensions = v2u32(1920, 1080)) {
		return RenderTarget::create(ctx,
			{
				RenderTarget::Attachement{
					.slot = RenderTarget::Color[0],
					.texture = TexBuffer::create(ctx, TexType::TX2D, v4u32(dimensions, 1, 1), GPUFormat::RGBA32F),
					.mip = 0,
					.layer = 0
				},
				RenderTarget::Attachement{
					.slot = RenderTarget::Depth,
					.texture = TexBuffer::create(ctx, TexType::TX2D, v4u32(dimensions, 1, 1), GPUFormat::DEPTH_COMPONENT32),
					.mip = 0,
					.layer = 0
				}
			}
		);
	}

	bool is_complete() const { return GL_GUARD(glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER)) == GL_FRAMEBUFFER_COMPLETE; }

	struct Image {
		TexBuffer* texture;
		GLint mip;
		GLint layer;
	};

	void bind() const { GL_GUARD(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer)); }
	static void unbind() { GL_GUARD(glBindFramebuffer(GL_FRAMEBUFFER, 0)); }

};

void blit_fb(const RenderTarget& source, const RenderTarget& target, rtu32 from = { v2u32(0), v2u32(0) }, rtu32 to = { v2u32(0), v2u32(0) }, GLbitfield mask = GL_COLOR_BUFFER_BIT, SamplingFilter filter = SamplingFilter::Nearest) {
	if (from.size() == v2u32(0))
		from = { v2u32(0, 0), source.dimensions };
	if (to.size() == v2u32(0))
		to = { v2u32(0, 0), target.dimensions };

	GL_GUARD(glBlitNamedFramebuffer(
		source.framebuffer, target.framebuffer,
		from.min.x, from.min.y, from.max.x, from.max.y,
		to.min.x, to.min.y, to.max.x, to.max.y,
		mask, filter
	));
}

struct RenderPass {
	GLuint framebuffer = 0;
	rtu32 viewport = { v2u32(0, 0), v2u32(1920, 1080) };
	rtu32 scissor = { v2u32(0, 0), v2u32(1920, 1080) };

	rtu32 draw_region() const { return intersect(viewport, scissor); }
	static RenderPass dflt;
};

rtu32 start_render_pass(const RenderPass& rp) {
	GL_GUARD(glBindFramebuffer(GL_FRAMEBUFFER, rp.framebuffer));
	GL_GUARD(glViewport(rp.viewport.min.x, rp.viewport.min.y, rp.viewport.max.x - rp.viewport.min.x, rp.viewport.max.y - rp.viewport.min.y));
	GL_GUARD(glScissor(rp.scissor.min.x, rp.scissor.min.y, rp.scissor.max.x - rp.scissor.min.x, rp.scissor.max.y - rp.scissor.min.y));
	return rp.draw_region();
}

RenderPass render_target_pass(const RenderTarget& target, rtu32 viewport = {}, rtu32 scissor = {}) {
	if (viewport.size() == v2u32(0)) viewport = { v2u32(0, 0), target.dimensions };
	if (scissor.size() == v2u32(0)) scissor = { v2u32(0, 0), target.dimensions };
	return {
		.framebuffer = target.framebuffer,
		.viewport = viewport,
		.scissor = scissor,
	};
}

enum : u32 {
	FLEX_CONTAINED = 1,
	FLEX_CONTAINING = 2,
	FLEX_CENTER = 4,
};

rtu32 center_rect(v2u32 size, rtu32 rect) {
	v2u32 offset = (size - rect.size()) / 2u;
	return {
		.min = rect.min + offset,
		.max = rect.max + offset
	};
}

rtu32 flex_viewport(v2u32 size, v2u32 aspect_ratio = v2u32(16, 9), u32 flags = FLEX_CONTAINED | FLEX_CENTER) {
	auto r = v2f32(size) / v2f32(aspect_ratio);
	f32 mul = 0;
	if (flags & FLEX_CONTAINED)
		mul = min(r.x, r.y);
	else if (flags & FLEX_CONTAINING)
		mul = max(r.x, r.y);
	if (flags & FLEX_CENTER) return center_rect(size, {
		.min = v2u32(0, 0),
		.max = v2f32(aspect_ratio) * v2f32(mul)
	});
	else return {
		.min = v2u32(0, 0),
		.max = v2f32(aspect_ratio) * v2f32(mul)
	};
}

bool EditorWidget(const cstr label, RenderTarget& target) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("dimensions", target.dimensions);
		changed |= EditorWidget("id", target.framebuffer);
		changed |= EditorWidget("attachments", target.attachments);
	}
	return changed;
}

bool EditorWidget(const cstr label, RenderPass& rp) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("framebuffer", rp.framebuffer);
		changed |= EditorWidget("viewport", rp.viewport);
		changed |= EditorWidget("scissor", rp.scissor);
	}
	return changed;
}

#define NamedEnum(t, e) tuple<const string, t>(#e, e)


#endif
