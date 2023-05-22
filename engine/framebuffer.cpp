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
};

inline AttachementBinding bind_to_fb(
	Attachement type,
	const TexBuffer& texture,
	GLint level,
	GLint layer
) {
	return AttachementBinding{ type, texture, level, layer };
}

tuple<bool, string> is_complete(GLuint fbf) {
	auto status = GL_GUARD(glCheckNamedFramebufferStatus(fbf, GL_FRAMEBUFFER));
	return tuple(status == GL_FRAMEBUFFER_COMPLETE, GLtoString(status));
}

GLuint create_framebuffer(Array<const AttachementBinding> attachements, bool must_complete = true) {
	GLuint id;
	GL_GUARD(glCreateFramebuffers(1, &id));
	for (auto&& attachement : attachements) {
		if (attachement.texture.type == TX2DARR || attachement.texture.type == TX3D)
			GL_GUARD(glNamedFramebufferTextureLayer(id, attachement.type, attachement.texture.id, attachement.level, attachement.layer));
		else
			GL_GUARD(glNamedFramebufferTexture(id, attachement.type, attachement.texture.id, attachement.level));
	}
	if (must_complete) {
		auto [complete, reason] = is_complete(id);
		if (!complete)
			fprintf(stderr, "Incomplete framebuffer %u : %s\n", id, reason.data());
	}
	return id;
}

GLuint create_framebuffer(LiteralArray<const AttachementBinding> attachements, bool must_complete = true) {
	return create_framebuffer(larray(attachements), must_complete);
}

GLuint& destroy_fb(GLuint& fb) {
	GL_GUARD(glDeleteFramebuffers(1, &fb));
	fb = 0;
	return fb;
}

#endif
