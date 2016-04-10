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

void PainterOpenGL::drawSprite(Material& material, Vector2f pos)
{
	init();

	// TODO: get current camera
	auto proj = Matrix4f::makeOrtho2D(0, 1280, 720, 0, -1000, 1000);

	for (size_t i = 0; i < material.getNumPasses(); i++) {
		// Bind camera and material
		material["u_mvp"] = proj;
		material.bind(i);

		// Set blend
		glUtils->setBlendType(material.getPass(i).getBlend());
		drawSpritePass(material.getAttributes(), pos);
	}
}

void PainterOpenGL::init()
{
	if (!glUtils) {
		glUtils = std::make_unique<GLUtils>();
	}
}

void PainterOpenGL::drawSpritePass(const std::vector<MaterialAttribute>& attributes, Vector2f pos)
{
	constexpr size_t numVertices = 4;
	const size_t numElements = 1;

	// HACK: read this elsewhere
	// Vertex attributes
	struct VertexAttrib
	{
		Vector2f pos;
		Vector2f offset;
		Vector2f size;
		Vector2f rotation;
		Colour4f colour;
		Vector2f texCoordMin;
		Vector2f texCoordMax;
		Vector2f vertPos;
	};
	VertexAttrib verts[numVertices];
	for (int v = 0; v < numVertices; v++) {
		auto& vert = verts[v];
		vert.pos = pos;
		vert.offset = Vector2f(0.5f, 0.5f);
		vert.size = Vector2f(64, 64);
		vert.rotation = Vector2f(0, 0);
		vert.colour = Colour4f(1, 1, 1, 1);
		vert.texCoordMin = Vector2f(0, 0);
		vert.texCoordMax = Vector2f(1, 1);
		const float vertPosX[] = { 0, 1, 1, 0 };
		const float vertPosY[] = { 0, 0, 1, 1 };
		vert.vertPos = Vector2f(vertPosX[v], vertPosY[v]);
	}

	// Compute data size and request VBO
	const size_t vertexStride = sizeof(VertexAttrib);
	const size_t elementStride = vertexStride * 4;
	const size_t bytesSize = elementStride * numElements;

	char* data = setupVBO(bytesSize);
	for (int v = 0; v < numVertices; v++) {
		char* dst = data + v * vertexStride;
		memcpy(dst, &verts[v], vertexStride);
	}

	setupVertexAttributes(attributes, numVertices, vertexStride, data);
	drawArraysQuads(numElements);
}

void PainterOpenGL::setupVertexAttributes(const std::vector<MaterialAttribute>& attributes, size_t numVertices, size_t vertexStride, char* vertexData)
{
	const size_t bytesSize = vertexStride * numVertices;

	// Load data into VBO
	glBufferData(GL_ARRAY_BUFFER, bytesSize, vertexData, GL_STREAM_DRAW);
	glCheckError();

	// Set vertex attribute pointers in VBO
	for (auto& attribute : attributes) {
		int count = 0;
		int type = 0;
		switch (attribute.type) {
		case AttributeType::Float:
			count = 1;
			type = GL_FLOAT;
			break;
		case AttributeType::Float2:
			count = 2;
			type = GL_FLOAT;
			break;
		case AttributeType::Float3:
			count = 3;
			type = GL_FLOAT;
			break;
		case AttributeType::Float4:
			count = 4;
			type = GL_FLOAT;
			break;
		}
		glEnableVertexAttribArray(attribute.location);
		glVertexAttribPointer(attribute.location, count, type, GL_FALSE, int(vertexStride), reinterpret_cast<GLvoid*>(attribute.offset));
		glCheckError();
	}

	// TODO: disable positions not used by this program
}

void PainterOpenGL::drawArraysQuads(int n)
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
