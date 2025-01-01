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

	v2u32 dimensions;
	GLuint framebuffer;
	Slot read_slot;
	Array<const Slot> draw_slots;

	static RenderTarget create(GLScope& ctx, v2u32 min_dimensions) {
		GLuint id;
		GL_GUARD(glCreateFramebuffers(1, &id));
		ctx.push<&GLScope::framebuffers>(id);
		return {
			.dimensions = v4u32(min_dimensions, 1, 1),
			.framebuffer = id,
			.read_slot = None,
			.draw_slots = {}
		};
	}

	bool is_complete() const { return GL_GUARD(glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER)) == GL_FRAMEBUFFER_COMPLETE; }

	RenderTarget& attach(const Slot& slt, const TexBuffer& texture, GLint mip = 0, GLint layer = 0) {
		dimensions = glm::max(v4u32(dimensions, 0, 0), texture.dimensions);
		switch (texture.type) {
			case TX2DARR: case TX3D: GL_GUARD(glNamedFramebufferTextureLayer(framebuffer, slt.id, texture.id, mip, layer)); break;
			default: GL_GUARD(glNamedFramebufferTexture(framebuffer, slt.id, texture.id, mip));
		}
		return *this;
	}

	struct Image {
		TexBuffer* texture;
		GLint mip;
		GLint layer;
	};

	RenderTarget& attach(const Slot& slt, const Image& img) { return attach(slt, *img.texture, img.mip, img.layer); }

	RenderTarget& attach(Array<const tuple<Slot, Image>> attachements) {
		for (auto&& [slt, img] : attachements)
			attach(slt, img);
		return *this;
	}

	Array<const Slot> select_draw_slots(Array<const Slot> slt) {
		auto last = draw_slots;
		GLenum buffers_b[32];
		auto buffers = map(larray(buffers_b), slt, [](const auto& t) -> GLenum { return t.id; });
		glNamedFramebufferDrawBuffers(framebuffer, buffers.size(), buffers.data());
		draw_slots = slt;
		return last;
	}

	Slot select_read_slot(Slot slt) {//* uses state-machine-like functionality, not super useful except debug of default fbf read
		auto last = read_slot;
		GL_GUARD(glNamedFramebufferReadBuffer(framebuffer, slt.id));
		read_slot = slt;
		return last;
	}

	void clear_attachement(GLint slot_index, v4f32 color = v4f32(v3f32(0.3), 1), GLint stencil = 0) {
		auto& target = draw_slots[slot_index];
		switch (target.type) {
			case GL_DEPTH: GL_GUARD(glClearNamedFramebufferfv(framebuffer, target.type, 0, &color.a)); break;
			case GL_STENCIL: GL_GUARD(glClearNamedFramebufferiv(framebuffer, target.type, 0, &stencil)); break;
			case GL_DEPTH_STENCIL: GL_GUARD(glClearNamedFramebufferfi(framebuffer, target.type, 0, color.a, stencil)); break;
			default: GL_GUARD(glClearNamedFramebufferfv(framebuffer, target.type, slot_index, glm::value_ptr(color)));
		}
	}

	void clear_slots(v4f32 color = v4f32(v3f32(0.3), 1), f32 depth = 1, GLint stencil = 0) {
		for (auto i = 0llu; i < draw_slots.size(); i++) {
			switch (draw_slots[i].type) {
				case GL_DEPTH: case GL_DEPTH_STENCIL: clear_attachement(i, v4f32(depth), stencil); continue;
				default: clear_attachement(i, color, stencil);
			}
		}
	}

	void bind() const { GL_GUARD(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer)); }
	static void unbind() { GL_GUARD(glBindFramebuffer(GL_FRAMEBUFFER, 0)); }

};

struct RenderPass {
	GLuint framebuffer = 0;
	rtu32 viewport = { v2u32(0, 0), v2u32(1920, 1080) };
	rtu32 scissor = { v2u32(0, 0), v2u32(1920, 1080) };

	static RenderPass dflt;
};

void start_render_pass(const RenderPass& rp) {
	GL_GUARD(glBindFramebuffer(GL_FRAMEBUFFER, rp.framebuffer));
	GL_GUARD(glViewport(rp.viewport.min.x, rp.viewport.min.y, rp.viewport.max.x - rp.viewport.min.x, rp.viewport.max.y - rp.viewport.min.y));
	GL_GUARD(glScissor(rp.scissor.min.x, rp.scissor.min.y, rp.scissor.max.x - rp.scissor.min.x, rp.scissor.max.y - rp.scissor.min.y));
}

bool EditorWidget(const cstr label, RenderTarget& target) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer{ ImGui::TreePop(); };
		changed |= EditorWidget("dimensions", target.dimensions);
		changed |= EditorWidget("id", target.framebuffer);
		changed |= EditorWidget("read_slot", target.read_slot);
		changed |= EditorWidget("draw_slots", target.draw_slots);
	}
	return changed;
}

bool EditorWidget(const cstr label, RenderPass& rp) {
	bool changed = false;
	if (ImGui::TreeNode(label)) {
		defer { ImGui::TreePop(); };
		changed |= EditorWidget("framebuffer", rp.framebuffer);
		changed |= EditorWidget("viewport", rp.viewport);
		changed |= EditorWidget("scissor", rp.scissor);
	}
	return changed;
}

#define NamedEnum(t, e) tuple<const string, t>(#e, e)


#endif
