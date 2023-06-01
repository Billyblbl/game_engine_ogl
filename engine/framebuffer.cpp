#ifndef GFRAMEBUFFER
# define GFRAMEBUFFER

#include <GL/glew.h>
#include <math.cpp>
#include <glutils.cpp>
#include <textures.cpp>
#include <tuple>
#include <blblstd.hpp>

enum Attachement : GLuint {
	NoAttc = 0,
	DepthAttc = GL_DEPTH_ATTACHMENT,
	StencilAttc = GL_STENCIL_ATTACHMENT,
	DepthStencilAttc = GL_DEPTH_STENCIL_ATTACHMENT,
	Color0Attc = GL_COLOR_ATTACHMENT0,
};

GLbitfield clear_bit(Attachement attch) {
	switch (attch) {
	case NoAttc: return 0;
	case DepthAttc: return GL_DEPTH_BUFFER_BIT;
	case StencilAttc: return GL_STENCIL_BUFFER_BIT;
	case DepthStencilAttc: return GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	default: return GL_COLOR_BUFFER_BIT;
	}
}

constexpr GLuint MaxAttachements = GL_MAX_COLOR_ATTACHMENTS;

struct AttachementBinding {
	Attachement type;
	TexBuffer texture;
	GLint level;
	GLint layer;
	bool clear;
};

inline AttachementBinding bind_to_fb(
	Attachement type,
	const TexBuffer& texture,
	GLint level,
	GLint layer,
	bool clear = true
) {
	return AttachementBinding{ type, texture, level, layer, clear };
}

tuple<bool, string> is_complete(GLuint fbf) {
	auto status = GL_GUARD(glCheckNamedFramebufferStatus(fbf, GL_FRAMEBUFFER));
	return tuple(status == GL_FRAMEBUFFER_COMPLETE, GLtoString(status));
}

struct FrameBuffer {
	GLuint id;
	GLbitfield clear_attachement;
	v2u32 dimensions;
};

static FrameBuffer default_framebuffer = { 0 };

FrameBuffer create_framebuffer(Array<const AttachementBinding> attachements, bool must_complete = true) {
	GLuint id;
	auto dimensions = v4u32(0);
	GLbitfield clear_bits = 0;
	GL_GUARD(glCreateFramebuffers(1, &id));
	for (auto&& attachement : attachements) {
		dimensions = glm::max(dimensions, attachement.texture.dimensions);
		if (attachement.texture.type == TX2DARR || attachement.texture.type == TX3D)
			GL_GUARD(glNamedFramebufferTextureLayer(id, attachement.type, attachement.texture.id, attachement.level, attachement.layer));
		else
			GL_GUARD(glNamedFramebufferTexture(id, attachement.type, attachement.texture.id, attachement.level));
		if (attachement.clear)
			clear_bits |= clear_bit(attachement.type);
	}
	if (must_complete) {
		auto [complete, reason] = is_complete(id);
		if (!complete)
			fprintf(stderr, "Incomplete framebuffer %u : %s\n", id, reason.data());
	}
	return { id, clear_bits, dimensions };
}

FrameBuffer create_framebuffer(LiteralArray<const AttachementBinding> attachements, bool must_complete = true) {
	return create_framebuffer(larray(attachements), must_complete);
}

FrameBuffer& destroy_fb(FrameBuffer& fbf) {
	GL_GUARD(glDeleteFramebuffers(1, &fbf.id));
	fbf.id = 0;
	fbf.dimensions = v2u32(0);
	return fbf;
}

void begin_render(FrameBuffer& fbf, rtf32 viewport = rtf32{ v2f32(0), v2f32(1) }) {
	GL_GUARD(glBindFramebuffer(GL_FRAMEBUFFER, fbf.id));
	GL_GUARD(glViewport(
		viewport.min.x * fbf.dimensions.x,
		viewport.min.y * fbf.dimensions.y,
		width(viewport) * fbf.dimensions.x,
		height(viewport) * fbf.dimensions.y
	));
}

// should be optional for now
void end_render() {
	GL_GUARD(glBindFramebuffer(GL_FRAMEBUFFER, default_framebuffer.id));
}

void clear(FrameBuffer& fbf, v4f32 color = v4f32(0), GLbitfield bits = ~0) {
	GL_GUARD(glClearColor(color.r, color.g, color.b, color.a));
	GL_GUARD(glClear(fbf.clear_attachement & bits));
}


#endif
