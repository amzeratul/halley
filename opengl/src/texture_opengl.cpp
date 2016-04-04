#include "halley_gl.h"
#include "texture_opengl.h"

using namespace Halley;

TextureOpenGL::TextureOpenGL(TextureDescriptor& descriptor)
{
	//privateImpl = 
}

void TextureOpenGL::create(size_t w, size_t h, int bpp, int format, bool useMipMap, bool useFiltering)
{
	glCheckError();
	assert(w > 0);
	assert(h > 0);
	// Older video cards can't deal with textures larger than 2048x2048
	assert(w <= 2048);
	assert(h <= 2048);

	size = Vector2i(static_cast<int>(w), static_cast<int>(h));

	int filtering = useFiltering ? GL_LINEAR : GL_NEAREST;

	unsigned int id;
	glGenTextures(1, &id);
	
	//GLUtils glUtils;
	glCheckError();
	//glUtils.bindTexture(id);
	glBindTexture(GL_TEXTURE_2D, id); // TODO: is this right?

	//loader = TextureLoadQueue::getCurrent();

#ifdef WITH_OPENGL_ES
	GLuint pixFormat = GL_UNSIGNED_BYTE;
#else
	GLuint pixFormat = GL_UNSIGNED_BYTE;

	if (useMipMap) glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -1);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glCheckError();
#endif
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, useMipMap ? GL_LINEAR_MIPMAP_LINEAR : filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);

	if (format == GL_DEPTH_COMPONENT24) {
		//glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}

	std::vector<char> blank;
	blank.resize(w * h * bpp);
	GLuint format2 = format;
	if (format2 == GL_RGBA16F || format2 == GL_RGBA16) format2 = GL_RGBA;
	if (format2 == GL_DEPTH_COMPONENT24) format2 = GL_DEPTH_COMPONENT;
	glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, format2, pixFormat, blank.data());
	glCheckError();
}
