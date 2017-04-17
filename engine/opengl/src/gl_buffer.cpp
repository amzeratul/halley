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
	size = size_t(data.size_bytes());
	if (capacity < size) {
		capacity = nextPowerOf2(size);
	}
	bind();
	glBufferData(target, capacity, nullptr, GL_STREAM_DRAW);
	glBufferSubData(target, 0, size, data.data());

	glCheckError();
}

void GLBuffer::bind()
{
	glBindBuffer(target, name);
	glCheckError();
}

void GLBuffer::bindToTarget(GLuint index)
{
	glBindBufferRange(target, index, name, 0, size);
	glCheckError();
}
