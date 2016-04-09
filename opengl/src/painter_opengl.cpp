#include "painter_opengl.h"
#include "halley_gl.h"
#include "../../core/src/graphics/material.h"
#include "../../core/src/graphics/camera.h"

using namespace Halley;

PainterOpenGL::PainterOpenGL()
{}

PainterOpenGL::~PainterOpenGL() = default;

void PainterOpenGL::startRender()
{
}

void PainterOpenGL::endRender()
{
	
}

void PainterOpenGL::clear(Colour colour)
{
	glClearColor(colour.r, colour.g, colour.b, colour.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void PainterOpenGL::drawSprite(Material& material, Vector2f pos)
{
	draw(material, pos);
}

void PainterOpenGL::init()
{
	if (!glUtils) {
		glUtils = std::make_unique<GLUtils>();
	}
}

void PainterOpenGL::draw(Material& material, Vector2f pos)
{
	init();

	// TODO: get current camera
	auto proj = Matrix4f::makeOrtho2D(0, 1280, 720, 0, -1000, 1000);

	// Bind camera and material
	material["u_mvp"] = proj;
	material.bind();

	// Set blend
	glUtils->setBlendType(material.getBlend());

	// TODO: read this elsewhere
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
	VertexAttrib vert;
	vert.pos = pos;
	vert.offset = Vector2f(0.5f, 0.5f);
	vert.size = Vector2f(64, 64);
	vert.rotation = Vector2f(0, 0);
	vert.colour = Colour4f(1, 1, 1, 1);
	vert.texCoordMin = Vector2f(0, 0);
	vert.texCoordMax = Vector2f(1, 1);

	// Compute data size and request VBO
	const size_t elements = 1;
	const size_t vertexStride = sizeof(VertexAttrib);
	const size_t spriteStride = vertexStride * 4;
	const size_t bytesSize = spriteStride * elements;
	char* data = setupVBO(bytesSize);

	// Load vertex data into VBO
	const float vertPosX[] = { 0, 1, 1, 0 };
	const float vertPosY[] = { 0, 0, 1, 1 };
	for (int v = 0; v < 4; v++) {
		char* dst = data + v * vertexStride;
		vert.vertPos = Vector2f(vertPosX[v], vertPosY[v]);
		memcpy(dst, &vert, vertexStride);
	}
	glBufferData(GL_ARRAY_BUFFER, bytesSize, data, GL_STREAM_DRAW);
	glCheckError();

	// Set vertex attribute pointers in VBO
	auto& shader = material.getShader();
	auto bindAttrib = [&] (const char* name, size_t count, GLuint type, size_t offset) {
		int loc = shader.getAttributeLocation(name);
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc, int(count), type, GL_FALSE, int(vertexStride), reinterpret_cast<GLvoid*>(offset));
		glCheckError();
	};

	bindAttrib("a_position", 4, GL_FLOAT, 0);
	bindAttrib("a_size", 4, GL_FLOAT, 4 * sizeof(float));
	bindAttrib("a_color", 4, GL_FLOAT, 8 * sizeof(float));
	bindAttrib("a_texCoord0", 4, GL_FLOAT, 12 * sizeof(float));
	bindAttrib("a_vertPos", 2, GL_FLOAT, 16 * sizeof(float));

	// Draw quads
	drawArraysQuads(elements);
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

	static std::vector<char> tmpData;
	if (tmpData.size() < size) {
		tmpData.resize(size * 2);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glCheckError();

	return tmpData.data();
}
