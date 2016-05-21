#include "../../utils/include/halley/support/debug.h"
#include <map>
#include "../../utils/include/halley/support/exception.h"
#include "../include/halley/opengl/render_target_opengl.h"

using namespace Halley;

RenderTargetOpenGL::~RenderTargetOpenGL()
{
	deInit();
}

Rect4i RenderTargetOpenGL::getViewPort() const
{
	return Rect4i(Vector2i(0, 0), attachments[0]->getSize());
}

void RenderTargetOpenGL::bind()
{
	Debug::trace("TextureRenderTargetFBO::bind begin");
	init();
	assert(fbo != 0);

	static GLuint buffers[] = { GL_COLOR_ATTACHMENT0 , GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7, GL_COLOR_ATTACHMENT8 };
	glDrawBuffers(int(attachments.size()), buffers);
	glCheckError();

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glCheckError();
	Debug::trace("TextureRenderTargetFBO::bind end");
}

void RenderTargetOpenGL::unbind()
{
	Debug::trace("RenderTargetOpenGL::unbind begin");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glCheckError();
	glDrawBuffer(GL_BACK);
	glCheckError();
	Debug::trace("RenderTargetOpenGL::unbind end");
}

void RenderTargetOpenGL::init()
{
	if (fbo == 0) {
		Debug::trace("RenderTargetOpenGL::init begin");

		glGenFramebuffers(1, &fbo);
		glCheckError();
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glCheckError();

		for (size_t i = 0; i < attachments.size(); i++) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + int(i), GL_TEXTURE_2D, attachments[i]->getNativeId(), 0);
			glCheckError();
		}
		if (depth) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth->getNativeId(), 0);
			glCheckError();
		}

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		glCheckError();
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			fbo = 0;
			std::map<int, String> msgs;
			msgs[GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT] = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
			msgs[GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT] = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
			msgs[GL_FRAMEBUFFER_UNSUPPORTED] = "GL_FRAMEBUFFER_UNSUPPORTED";
			throw Exception("Unable to set up framebuffer: error " + msgs[status]);
		}

		Debug::trace("RenderTargetOpenGL::init end");
	}
}

void RenderTargetOpenGL::deInit()
{
	if (fbo != 0) {
		unbind();
	}
}
