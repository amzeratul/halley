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

		void clear(Colour colour) override;
		void setBlend(BlendType blend) override;

	protected:
		void setVertices(Material& material, size_t numVertices, void* vertexData) override;
		void drawQuads(size_t n) override;
		void setViewPort(Rect4i rect, bool enableScissor) override;

	private:
		GLuint vbo = 0;
		GLuint vao = 0;
		std::unique_ptr<GLUtils> glUtils;

		std::vector<char> vboData;
		std::vector<unsigned short> indexData;

		void setupVertexAttributes(Material& material);
		char* getVBOBuffer(size_t size);
	};
}
