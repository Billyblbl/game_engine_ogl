#ifndef GFRAMEBUFFER
# define GFRAMEBUFFER

#include <GL/glew.h>
#include <math.cpp>
#include <glutils.cpp>
#include <textures.cpp>
#include <tuple>
#include <blblstd.hpp>

struct Framebuffer {
	GLuint id;
	v2u32 pixel_dimensions;

	enum Attachement: GLuint {
		Depth = GL_DEPTH_ATTACHMENT,
		Stencil = GL_STENCIL_ATTACHMENT,
		DepthStencil = GL_DEPTH_STENCIL_ATTACHMENT,
		Color0 = GL_COLOR_ATTACHMENT0,
		Color1 = GL_COLOR_ATTACHMENT1,
		Color2 = GL_COLOR_ATTACHMENT2,
		Color3 = GL_COLOR_ATTACHMENT3,
		Color4 = GL_COLOR_ATTACHMENT4,
		Color5 = GL_COLOR_ATTACHMENT5,
		Color6 = GL_COLOR_ATTACHMENT6,
		Color7 = GL_COLOR_ATTACHMENT7,
		Color8 = GL_COLOR_ATTACHMENT8,
		Color9 = GL_COLOR_ATTACHMENT9,
		Color10 = GL_COLOR_ATTACHMENT10,
		Color11 = GL_COLOR_ATTACHMENT11,
		Color12 = GL_COLOR_ATTACHMENT12,
		Color13 = GL_COLOR_ATTACHMENT13,
		Color14 = GL_COLOR_ATTACHMENT14,
		Color15 = GL_COLOR_ATTACHMENT15,
	};

	static constexpr GLuint MaxAttachements = 18;

};

Framebuffer create_framebuffer(Array<std::pair<Framebuffer::Attachement, Textures::Texture*>> attachements) {
	Framebuffer buffer;
	GL_GUARD(glCreateFramebuffers(1, &buffer.id));
	for (auto&& attachement : attachements) {
		GL_GUARD(glNamedFramebufferTexture(buffer.id, attachement.first, attachement.second->id, 0));
	}
	auto status = GL_GUARD(glCheckNamedFramebufferStatus(buffer.id, GL_FRAMEBUFFER));
	assert(status == GL_FRAMEBUFFER_COMPLETE);
	return buffer;
}

Framebuffer create_framebuffer(
	Array<Textures::Texture*> colors = {},
	Textures::Texture* depth = nullptr,
	Textures::Texture* stencil = nullptr,
	Textures::Texture* depthStencil = nullptr
) {
	std::pair<Framebuffer::Attachement, Textures::Texture*> buff[Framebuffer::MaxAttachements];
	auto attachements = List { larray(buff), 0 };

	for (auto&& colorAtt : colors) {
		attachements.push(std::make_pair((Framebuffer::Attachement)(Framebuffer::Color0 + attachements.current), colorAtt));
	}

	if (depth != nullptr)
		attachements.push(std::make_pair(Framebuffer::Depth, depth));
	if (stencil != nullptr)
		attachements.push(std::make_pair(Framebuffer::Stencil, stencil));
	if (depthStencil != nullptr)
		attachements.push(std::make_pair(Framebuffer::DepthStencil, depthStencil));
	return create_framebuffer(attachements.allocated());
}

Framebuffer create_framebuffer(Textures::Texture& color) {
	Textures::Texture* colorPtr = &color;
	return create_framebuffer(std::span(&colorPtr, 1));
}

//TODO framebuffer destruction
#endif
