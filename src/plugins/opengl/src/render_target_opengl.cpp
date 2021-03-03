#include "halley/support/debug.h"
#include "halley/support/exception.h"
#include "render_target_opengl.h"
#include <halley/core/graphics/texture.h>
#include <halley/data_structures/flat_map.h>
#include <gsl/gsl_assert>
#include "texture_opengl.h"

using namespace Halley;

TextureRenderTargetOpenGL::~TextureRenderTargetOpenGL()
{
	deInit();
}

void TextureRenderTargetOpenGL::onBind(Painter&)
{
	init();
	Expects(fbo != 0);
	
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glCheckError();

#ifdef WITH_OPENGL
	static GLuint buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7, GL_COLOR_ATTACHMENT8 };
	glDrawBuffers(int(colourBuffer.size()), buffers);
#else
	// TODO?
	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, );
#endif

	glCheckError();
}

void TextureRenderTargetOpenGL::onUnbind(Painter&)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TextureRenderTargetOpenGL::init()
{
	if (fbo == 0) {
		HALLEY_DEBUG_TRACE();

		glGenFramebuffers(1, &fbo);
		glCheckError();
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glCheckError();

		for (size_t i = 0; i < colourBuffer.size(); i++) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + int(i), GL_TEXTURE_2D, dynamic_cast<TextureOpenGL&>(*colourBuffer[i]).getNativeId(), 0);
			glCheckError();
		}
		if (depthStencilBuffer) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dynamic_cast<TextureOpenGL&>(*depthStencilBuffer).getNativeId(), 0);
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
			throw Exception("Unable to set up framebuffer: error " + msgs[status], HalleyExceptions::VideoPlugin);
		}

		HALLEY_DEBUG_TRACE();
	}
}

void TextureRenderTargetOpenGL::deInit()
{
	if (fbo != 0) {
		glDeleteFramebuffers(1, &fbo);
		fbo = 0;
	}
}

bool ScreenRenderTargetOpenGL::getProjectionFlipVertical() const
{
	return true;
}

bool ScreenRenderTargetOpenGL::getViewportFlipVertical() const
{
	return true;
}

void ScreenRenderTargetOpenGL::onBind(Painter&)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#ifdef WITH_OPENGL
	GLuint buffer = GL_BACK_LEFT;
	glDrawBuffers(1, &buffer);
#else
	// TODO?
#endif
}
