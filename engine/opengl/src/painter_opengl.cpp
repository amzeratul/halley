#include "painter_opengl.h"
#include "halley/core/graphics/material/material_definition.h"
#include <gsl/gsl_assert>

using namespace Halley;

PainterOpenGL::PainterOpenGL()
{}

PainterOpenGL::~PainterOpenGL()
{
	if (vbo != 0) {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}
	if (veo != 0) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glDeleteBuffers(1, &veo);
		veo = 0;
	}

#ifdef WITH_OPENGL
	if (vao != 0) {
		glBindVertexArray(0);
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
#endif
}

void PainterOpenGL::doStartRender()
{
	glCheckError();

	if (!glUtils) {
		glUtils = std::make_unique<GLUtils>();
	}

	if (vbo == 0) {
		glGenBuffers(1, &vbo);
		glCheckError();
	}

	if (veo == 0) {
		glGenBuffers(1, &veo);
		glCheckError();
	}

#ifdef WITH_OPENGL
	if (vao == 0) {
		glGenVertexArrays(1, &vao);
		glCheckError();
		glBindVertexArray(vao);
		glCheckError();
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glCheckError();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, veo);
		glCheckError();
	}

	glBindVertexArray(vao);
#endif
}

void PainterOpenGL::doEndRender()
{
#ifdef WITH_OPENGL
	glBindVertexArray(0);
#endif
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

static Rect4i flipRectangle(Rect4i r, int h)
{
	int y = h - r.getBottom();
	return Rect4i(Vector2i(r.getLeft(), y), r.getWidth(), r.getHeight());
}

void PainterOpenGL::setClip(Rect4i clip, Vector2i renderTargetSize, bool enable, bool isScreen)
{
	if (isScreen) {
		glUtils->setScissor(flipRectangle(clip, renderTargetSize.y), enable);
	} else {
		glUtils->setScissor(clip, enable);
	}	
}

void PainterOpenGL::setViewPort(Rect4i rect, Vector2i renderTargetSize, bool isScreen)
{
	if (isScreen) {
		glUtils->setViewPort(flipRectangle(rect, renderTargetSize.y));
	} else {
		glUtils->setViewPort(rect);
	}
}

void PainterOpenGL::setVertices(MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices)
{
	Expects(numVertices > 0);
	Expects(numIndices >= numVertices);
	Expects(vertexData);
	Expects(indices);

	// Load indices into VBO
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, int(numIndices) * sizeof(unsigned short), indices, GL_STREAM_DRAW);
	glCheckError();

	// Load vertices into VBO
	size_t bytesSize = numVertices * material.getVertexStride();
	glBufferData(GL_ARRAY_BUFFER, bytesSize, vertexData, GL_STREAM_DRAW);
	glCheckError();

	// Set attributes
	setupVertexAttributes(material);
}

void PainterOpenGL::setupVertexAttributes(MaterialDefinition& material)
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
		case ShaderParameterType::Int:
			count = 1;
			type = GL_INT;
			break;
		case ShaderParameterType::Int2:
			count = 2;
			type = GL_INT;
			break;
		case ShaderParameterType::Int3:
			count = 3;
			type = GL_INT;
			break;
		case ShaderParameterType::Int4:
			count = 4;
			type = GL_INT;
			break;
		default:
			break;
		}
		glEnableVertexAttribArray(attribute.location);
		size_t offset = attribute.offset;
		glVertexAttribPointer(attribute.location, count, type, GL_FALSE, vertexStride, reinterpret_cast<GLvoid*>(offset));
		glCheckError();
	}

	// TODO: disable positions not used by this program
}

void PainterOpenGL::drawTriangles(size_t numIndices)
{
	Expects(numIndices > 0);
	Expects(numIndices % 3 == 0);

	glDrawElements(GL_TRIANGLES, int(numIndices), GL_UNSIGNED_SHORT, nullptr);
	glCheckError();
}
