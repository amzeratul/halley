#pragma once

#include "halley_gl.h"

namespace Halley
{
	class MaterialPass;

	class PainterOpenGL : public Painter
	{
	public:
		PainterOpenGL();
		~PainterOpenGL();

		void startRender() override;
		void endRender() override;

		void drawVertices(Material& material, size_t numVertices, size_t vertexStride, void* vertexData) override;
		void clear(Colour colour) override;

	private:
		GLuint vbo = 0;
		GLuint vao = 0;
		std::unique_ptr<GLUtils> glUtils;

		void init();
		void setupVertexAttributes(const std::vector<MaterialAttribute>& attributes, size_t numVertices, size_t elementStride, char* vertexData);
		void drawArraysQuads(int n);
		char* setupVBO(size_t size);
	};
}
