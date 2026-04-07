#include "structured_buffer_opengl.h"
#include "gl_utils.h"
#include "halley_gl.h"

#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif

using namespace Halley;

StructuredBufferOpenGL::StructuredBufferOpenGL()
{
	buffer.init(GL_SHADER_STORAGE_BUFFER);
}

void StructuredBufferOpenGL::update(gsl::span<const std::byte> data, size_t elementStride)
{
	buffer.setData(data);
	dataSize = data.size_bytes();
	stride = elementStride;
}

void StructuredBufferOpenGL::bind(int bindPoint)
{
	buffer.bindToTarget(bindPoint);
	glCheckError();
}
