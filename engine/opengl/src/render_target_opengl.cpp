#include "halley/support/debug.h"
#include "halley/support/exception.h"
#include "render_target_opengl.h"
#include <halley/core/graphics/texture.h>
#include <halley/data_structures/flat_map.h>
#include <gsl/gsl_assert>

using namespace Halley;

RenderTargetOpenGL::~RenderTargetOpenGL()
{
	deInit();
}

void RenderTargetOpenGL::bind()
{
	init();
	Expects(fbo != 0);
	
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glCheckError();

#ifdef WITH_OPENGL
	static GLuint buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7, GL_COLOR_ATTACHMENT8 };
	glDrawBuffers(int(attachments.size()), buffers);
#else
	// TODO?
	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, );
#endif

	glCheckError();
}

void RenderTargetOpenGL::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glCheckError();
#ifdef WITH_OPENGL
	static GLuint buffers[] = { GL_BACK_LEFT };
	glDrawBuffers(1, buffers);
#else
	// TODO?
#endif
	glCheckError();
}

void RenderTargetOpenGL::init()
{
	if (fbo == 0) {
		HALLEY_DEBUG_TRACE();

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
			FlatMap<int, String> msgs;
			msgs[GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT] = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
			msgs[GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT] = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
			msgs[GL_FRAMEBUFFER_UNSUPPORTED] = "GL_FRAMEBUFFER_UNSUPPORTED";
			throw Exception("Unable to set up framebuffer: error " + msgs[status]);
		}

		HALLEY_DEBUG_TRACE();
	}
}

void RenderTargetOpenGL::deInit()
{
	if (fbo != 0) {
		unbind();
	}
}
