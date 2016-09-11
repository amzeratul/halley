#include "halley/concurrency/concurrent.h"
#include "halley/support/exception.h"
#include "halley_gl.h"
#include "texture_opengl.h"
#include "halley/core/graphics/texture_descriptor.h"
#include <gsl/gsl_assert>
#include "video_opengl.h"

using namespace Halley;

TextureOpenGL::TextureOpenGL(VideoOpenGL& parent, Vector2i s)
	: parent(parent)
	, loaded(false)
{
	size = s;

	glGenTextures(1, &textureId);
}

TextureOpenGL::~TextureOpenGL()
{
	waitForLoad();
	glDeleteTextures(1, &textureId);
	textureId = 0;
}

void TextureOpenGL::load(const TextureDescriptor& d)
{
	create(d.size.x, d.size.y, d.format, d.useMipMap, d.useFiltering);
	if (d.pixelData != nullptr) {
		loadImage(reinterpret_cast<const char*>(d.pixelData), d.size.x, d.size.y, d.size.x, d.format, d.useMipMap);
	}

	if (parent.isLoaderThread()) {
		fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		glFlush();
	}

	loaded = true;
	loadWait.notify_all();
}

void TextureOpenGL::waitForLoad()
{
	if (!loaded) {
		std::unique_lock<std::mutex> lock(loadMutex);
		loadWait.wait(lock);
	}

	if (fence) {
		GLenum result = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, GLuint64(10000000000));
		if (result == GL_TIMEOUT_EXPIRED) {
			throw Exception("Timeout waiting for texture to load.");
		}
		else if (result == GL_WAIT_FAILED) {
			throw Exception("Error waiting for texture to load.");
		}
		else if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
			glDeleteSync(fence);
			fence = nullptr;
		}
	}
}

bool TextureOpenGL::isLoaded() const
{
	return loaded;
}

// Do not use this method inside this class, due to fence
void TextureOpenGL::bind(int textureUnit)
{
	waitForLoad();
	GLUtils glUtils;
	glUtils.setTextureUnit(textureUnit);
	glUtils.bindTexture(textureId);
}

void TextureOpenGL::create(size_t w, size_t h, TextureFormat format, bool useMipMap, bool useFiltering)
{
	Expects(w > 0);
	Expects(h > 0);
	Expects(w <= 4096);
	Expects(h <= 4096);
	glCheckError();

	int filtering = useFiltering ? GL_LINEAR : GL_NEAREST;

	GLUtils glUtils;
	glUtils.bindTexture(textureId);

	//loader = TextureLoadQueue::getCurrent();

#ifdef WITH_OPENGL
	GLuint pixFormat = GL_UNSIGNED_BYTE;

	if (useMipMap) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glCheckError();
#else
	GLuint pixFormat = GL_UNSIGNED_BYTE;
#endif
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, useMipMap ? GL_LINEAR_MIPMAP_LINEAR : filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);

#ifdef WITH_OPENGL
	if (format == TextureFormat::DEPTH) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}
#endif

	Vector<char> blank;
	blank.resize(w * h * TextureDescriptor::getBitsPerPixel(format));
	GLuint glFormat = getGLFormat(format);
	GLuint format2 = glFormat;
#ifdef WITH_OPENGL
	if (format2 == GL_RGBA16F || format2 == GL_RGBA16) format2 = GL_RGBA;
	if (format2 == GL_DEPTH_COMPONENT24) format2 = GL_DEPTH_COMPONENT;
#else
	if (format2 == GL_DEPTH_COMPONENT16) format2 = GL_DEPTH_COMPONENT;
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, glFormat, size.x, size.y, 0, format2, pixFormat, blank.data());
	glCheckError();
}

void TextureOpenGL::loadImage(const char* px, size_t w, size_t h, size_t stride, TextureFormat format, bool useMipMap)
{
#ifdef WITH_OPENGL
	glPixelStorei(GL_PACK_ROW_LENGTH, int(stride));
#endif
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, int(w), int(h), getGLFormat(format), GL_UNSIGNED_BYTE, px);
	glCheckError();

#ifndef WITH_OPENGL_ES

	// Generate mipmap
	if (useMipMap) {
		glGenerateMipmap(GL_TEXTURE_2D);
		glCheckError();
	}

#endif

	if (Concurrent::getThreadName() != "main") {
		glFinish();
	}
}

unsigned TextureOpenGL::getGLFormat(TextureFormat format)
{
	switch (format) {
	case TextureFormat::RGB:
		return GL_RGB;
	case TextureFormat::RGBA:
		return GL_RGBA;
	case TextureFormat::DEPTH:
#ifdef WITH_OPENGL
		return GL_DEPTH_COMPONENT24;
#else
		return GL_DEPTH_COMPONENT16;
#endif
	}
	throw Exception("Unknown texture format: " + String::integerToString(static_cast<int>(format)));
}
