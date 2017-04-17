#include "gl_buffer.h"

using namespace Halley;

GLBuffer::GLBuffer() {}

GLBuffer::~GLBuffer()
{
	if (name != 0) {
		glBindBuffer(target, 0);
		glDeleteBuffers(1, &name);
	}
}

void GLBuffer::init(GLenum t)
{
	if (name == 0) {
		target = t;
		glGenBuffers(1, &name);
	}
}

void GLBuffer::setData(gsl::span<const gsl::byte> data)
{
	if (size < size_t(data.size_bytes())) {
		size = nextPowerOf2(data.size_bytes());
	}
	bind();
	glBufferData(target, size, nullptr, GL_STREAM_DRAW);
	glBufferSubData(target, 0, data.size_bytes(), data.data());

	glCheckError();
}

void GLBuffer::bind()
{
	glBindBuffer(target, name);
	glCheckError();
}

void GLBuffer::bindToTarget(GLuint index)
{
	glBindBufferBase(target, index, name);
	glCheckError();
}
