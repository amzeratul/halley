#include "halley/concurrency/concurrent.h"
#include "halley/support/exception.h"
#include "halley_gl.h"
#include "texture_opengl.h"
#include "halley/graphics/texture_descriptor.h"
#include <gsl/assert>
#include "video_opengl.h"
#include "halley/game/game_platform.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

using namespace Halley;

TextureOpenGL::TextureOpenGL(VideoOpenGL& parent, Vector2i size)
	: Texture(size)
	, parent(parent)
{
	glGenTextures(1, &textureId);
	assert(textureId != 0);
	startLoading();
}

TextureOpenGL::~TextureOpenGL()
{
	waitForOpenGLLoad();
	if (textureId != 0) {
		glDeleteTextures(1, &textureId);
		textureId = 0;
	}
}

TextureOpenGL& TextureOpenGL::operator=(TextureOpenGL&& other) noexcept
{
	other.waitForOpenGLLoad();

	moveFrom(other);

	textureId = other.textureId;
	texSize = other.texSize;

	other.textureId = 0;
	other.texSize = {};

	doneLoading();

	return *this;
}

void TextureOpenGL::doLoad(TextureDescriptor& d)
{
	GLUtils glUtils;
    glUtils.setTextureUnit(0);
	glUtils.bindTexture(textureId);
	
	if (texSize != d.size) {
		create(d.size, d.format, d.useMipMap, d.useFiltering, d.addressMode, d.pixelData);
	} else if (!d.pixelData.empty()) {
		updateImage(d.pixelData, d.format, d.useMipMap);
	}
	finishLoading();
}

void TextureOpenGL::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<TextureOpenGL&>(resource));
}

void TextureOpenGL::generateMipMaps()
{
	if (descriptor.useMipMap) {
		GLUtils glUtils;
	    glUtils.setTextureUnit(0);
		glUtils.bindTexture(textureId);

		glGenerateMipmap(GL_TEXTURE_2D);
		glCheckError();
	}
}

unsigned TextureOpenGL::getNativeId() const
{
	return textureId;
}

void TextureOpenGL::finishLoading()
{
	if (parent.isLoaderThread()) {
		if constexpr (getPlatform() != GamePlatform::Emscripten) {
			fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		}
		glFlush();
	}

	doneLoading();
}

void TextureOpenGL::waitForOpenGLLoad() const
{
	waitForLoad();

	if (fence) {
		GLuint64 timeout = GLuint64(10000000000);
		GLenum result = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);

		while (result == GL_TIMEOUT_EXPIRED) {
			std::this_thread::yield();
			result = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
		}

		if (result == GL_WAIT_FAILED) {
			glCheckError();
		} else if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
			glDeleteSync(fence);
			fence = nullptr;
		}
	}
}

// Do not use this method inside this class, due to fence
void TextureOpenGL::bind(int textureUnit) const
{
	waitForOpenGLLoad();
	GLUtils glUtils;
	glUtils.setTextureUnit(textureUnit);
	glUtils.bindTexture(textureId);
}

void TextureOpenGL::create(Vector2i size, TextureFormat format, bool useMipMap, bool useFiltering, TextureAddressMode addressMode, TextureDescriptorImageData& pixelData)
{
	Expects(size.x > 0);
	Expects(size.y > 0);
	//Expects(size.x <= 4096);
	//Expects(size.y <= 4096);
	glCheckError();

#if defined (WITH_OPENGL)
	if (useMipMap) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -1);
	}
#endif

	const auto wrap = getGLAddressMode(addressMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

	const int filtering = useFiltering ? GL_LINEAR : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, useMipMap ? GL_LINEAR_MIPMAP_LINEAR : filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);

	if (format == TextureFormat::Depth) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}

	const auto internalFormat = getGLInternalFormat(format);
	const auto pixelFormat = getGLPixelFormat(format);
	const auto byteFormat = getGLByteFormat(format);

	if (format != TextureFormat::Depth) {
		const int stride = pixelData.empty() ? size.x : pixelData.getStrideOr(size.x);
		glPixelStorei(GL_UNPACK_ALIGNMENT, TextureDescriptor::getBytesPerPixel(format) == 4 ? 4 : 1);
		glPixelStorei(GL_PACK_ROW_LENGTH, stride);
		glCheckError();
	}

	if (pixelData.empty()) {
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.x, size.y, 0, pixelFormat, byteFormat, nullptr);
	} else {
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.x, size.y, 0, pixelFormat, byteFormat, pixelData.getBytes());
	}
	glCheckError();

#ifdef WITH_OPENGL
	glObjectLabel(GL_TEXTURE, textureId, -1, getAssetId().c_str());
#endif

	texSize = size;
}

void TextureOpenGL::updateImage(TextureDescriptorImageData& pixelData, TextureFormat format, bool useMipMap)
{
	int stride = pixelData.getStrideOr(size.x);

	glPixelStorei(GL_UNPACK_ALIGNMENT, TextureDescriptor::getBytesPerPixel(format));
	glPixelStorei(GL_PACK_ROW_LENGTH, stride);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y, getGLPixelFormat(format), GL_UNSIGNED_BYTE, pixelData.getBytes());
	glCheckError();

	// Generate mipmap
	if (useMipMap) {
		glGenerateMipmap(GL_TEXTURE_2D);
		glCheckError();
	}
}

int TextureOpenGL::getGLInternalFormat(TextureFormat format)
{
	switch (format) {
	case TextureFormat::Indexed:
	case TextureFormat::Red:
		return GL_R8;
	case TextureFormat::RGB:
		return GL_RGB8;
	case TextureFormat::RGBA:
		return GL_RGBA8;
	case TextureFormat::Depth:
		return GL_DEPTH24_STENCIL8;
	default:
		throw Exception("Unknown texture format: " + toString(static_cast<int>(format)), HalleyExceptions::VideoPlugin);
	}
}

unsigned TextureOpenGL::getGLPixelFormat(TextureFormat format)
{
	switch (format) {
	case TextureFormat::Indexed:
	case TextureFormat::Red:
		return GL_RED;
	case TextureFormat::RGB:
		return GL_RGB;
	case TextureFormat::RGBA:
		return GL_RGBA;
	case TextureFormat::Depth:
		return GL_DEPTH_STENCIL;
	default:
		throw Exception("Unknown texture format: " + toString(static_cast<int>(format)), HalleyExceptions::VideoPlugin);
	}
}

unsigned TextureOpenGL::getGLByteFormat(TextureFormat format)
{
	switch (format) {
	case TextureFormat::Indexed:
	case TextureFormat::Red:
	case TextureFormat::RGB:
	case TextureFormat::RGBA:
		return GL_UNSIGNED_BYTE;
	case TextureFormat::Depth:
		return GL_UNSIGNED_INT_24_8;
	default:
		throw Exception("Unknown texture format: " + toString(static_cast<int>(format)), HalleyExceptions::VideoPlugin);
	}
}

int TextureOpenGL::getGLAddressMode(TextureAddressMode addressMode)
{
	switch (addressMode) {
	case TextureAddressMode::Clamp:
		return GL_CLAMP_TO_EDGE;
	case TextureAddressMode::Mirror:
		return GL_MIRRORED_REPEAT;
	case TextureAddressMode::Repeat:
		return GL_REPEAT;
#ifdef WITH_OPENGL
	case TextureAddressMode::Border:
		return GL_CLAMP_TO_BORDER;
#endif
	default:
		throw Exception("Unknown texture address mode: " + toString(addressMode), HalleyExceptions::VideoPlugin);
	}
}
