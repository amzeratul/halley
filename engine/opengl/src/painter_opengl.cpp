#include "painter_opengl.h"
#include "halley/core/graphics/material/material_definition.h"
#include <gsl/gsl_assert>
#include "shader_opengl.h"
#include "constant_buffer_opengl.h"

using namespace Halley;

PainterOpenGL::PainterOpenGL()
{}

PainterOpenGL::~PainterOpenGL()
{
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
	
	glUtils->setNumberOfTextureUnits(1);
	glUtils->bindTexture(0);
	glUtils->setScissor(Rect4i(), false);

	vertexBuffer.init(GL_ARRAY_BUFFER);
	elementBuffer.init(GL_ELEMENT_ARRAY_BUFFER);

#ifdef WITH_OPENGL
	if (vao == 0) {
		glGenVertexArrays(1, &vao);
		glCheckError();
		glBindVertexArray(vao);
		glCheckError();
		vertexBuffer.bind();
		elementBuffer.bind();
	}

	glBindVertexArray(vao);
#endif

	clear(Colour(0, 0, 0, 1.0f));
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

void PainterOpenGL::setMaterialPass(const Material& material, int passNumber)
{
	auto& pass = material.getDefinition().getPass(passNumber);

	glUtils->setBlendType(pass.getBlend());
	static_cast<ShaderOpenGL&>(pass.getShader()).bind();
	static_cast<ConstantBufferOpenGL&>(material.getConstantBuffer()).bind(passNumber);
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

void PainterOpenGL::setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices)
{
	Expects(numVertices > 0);
	Expects(numIndices >= numVertices);
	Expects(vertexData);
	Expects(indices);

	// Load indices into VBO
	elementBuffer.setData(gsl::as_bytes(gsl::span<unsigned short>(indices, numIndices)));

	// Load vertices into VBO
	size_t bytesSize = numVertices * material.getVertexStride();
	vertexBuffer.setData(gsl::as_bytes(gsl::span<char>(static_cast<char*>(vertexData), bytesSize)));

	// Set attributes
	setupVertexAttributes(material);
}

void PainterOpenGL::setupVertexAttributes(const MaterialDefinition& material)
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
