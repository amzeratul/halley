#include "painter_opengl.h"
#include "halley_gl.h"
#include "../../core/src/graphics/material.h"
#include "../../core/src/graphics/material_parameter.h"
#include "../../core/src/graphics/camera.h"

using namespace Halley;

PainterOpenGL::PainterOpenGL()
{}

PainterOpenGL::~PainterOpenGL()
{
	if (vbo != 0) {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDeleteBuffers(1, &vbo);
	}
	if (vao != 0) {
		glBindVertexArray(0);
		glDeleteVertexArrays(1, &vao);
	}
}

void PainterOpenGL::startRender()
{
	glCheckError();
}

void PainterOpenGL::endRender()
{
	glCheckError();
}

void PainterOpenGL::clear(Colour colour)
{
	glCheckError();
	glClearColor(colour.r, colour.g, colour.b, colour.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glCheckError();
}

void PainterOpenGL::setBlend(BlendType blend)
{
	glUtils->setBlendType(blend);
}

void PainterOpenGL::setVertices(Material& material, size_t numVertices, void* vertexData)
{
	init();

	// Load vertices into VBO
	size_t vertexStride = material.getVertexStride();
	size_t bytesSize = numVertices * vertexStride;
	char* data = setupVBO(bytesSize);
	memcpy(data, vertexData, bytesSize);
	glBufferData(GL_ARRAY_BUFFER, bytesSize, vertexData, GL_STREAM_DRAW);
	glCheckError();

	// Set attributes
	setupVertexAttributes(material);
}

void PainterOpenGL::init()
{
	if (!glUtils) {
		glUtils = std::make_unique<GLUtils>();
	}
}

void PainterOpenGL::setupVertexAttributes(Material& material)
{
	// Set vertex attribute pointers in VBO
	int vertexStride = material.getVertexStride();
	for (auto& attribute : material.getAttributes()) {
		int count = 0;
		int type = 0;
		switch (attribute.type) {
		case ShaderParameterType::Float:
			count = 1;
			type = GL_FLOAT;
			break;
		case ShaderParameterType::Float2:
			count = 2;
			type = GL_FLOAT;
			break;
		case ShaderParameterType::Float3:
			count = 3;
			type = GL_FLOAT;
			break;
		case ShaderParameterType::Float4:
			count = 4;
			type = GL_FLOAT;
			break;
		}
		glEnableVertexAttribArray(attribute.location);
		size_t offset = attribute.offset;
		glVertexAttribPointer(attribute.location, count, type, GL_FALSE, vertexStride, reinterpret_cast<GLvoid*>(offset));
		glCheckError();
	}

	// TODO: disable positions not used by this program
}

void PainterOpenGL::drawQuads(size_t n)
{
	size_t sz = n * 6;
	std::vector<unsigned short> indices(sz);
	unsigned short pos = 0;
	for (size_t i = 0; i<sz; i += 6) {
		// B-----C
		// |     |
		// A-----D
		// ABC
		indices[i] = pos;
		indices[i + 1] = pos + 1;
		indices[i + 2] = pos + 2;
		// CDA
		indices[i + 3] = pos + 2;
		indices[i + 4] = pos + 3;
		indices[i + 5] = pos;
		pos += 4;
	}
	glDrawElements(GL_TRIANGLES, int(sz), GL_UNSIGNED_SHORT, indices.data());
	glCheckError();
}

char* PainterOpenGL::setupVBO(size_t size)
{
	if (vbo == 0) {
		glGenBuffers(1, &vbo);
		glCheckError();
	}

	if (vao == 0) {
		glGenVertexArrays(1, &vao);
		glCheckError();
	}

	static std::vector<char> tmpData;
	if (tmpData.size() < size) {
		tmpData.resize(size * 2);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glCheckError();
	glBindVertexArray(vao);
	glCheckError();

	return tmpData.data();
}
