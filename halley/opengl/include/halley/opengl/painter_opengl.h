#pragma once

#include "halley/core/graphics/painter.h"
#include "halley_gl.h"

namespace Halley
{
	class MaterialPass;

	class PainterOpenGL final : public Painter
	{
	public:
		PainterOpenGL();
		~PainterOpenGL();

		void doStartRender() override;
		void doEndRender() override;

		void clear(Colour colour) override;
		void setBlend(BlendType blend) override;

	protected:
		void setVertices(MaterialDefinition& material, size_t numVertices, void* vertexData) override;
		void drawQuads(size_t n) override;
		void setViewPort(Rect4i rect, bool enableScissor) override;

	private:
		GLuint vbo = 0;
		GLuint vao = 0;
		GLuint veo = 0;
		std::unique_ptr<GLUtils> glUtils;

		void setupVertexAttributes(MaterialDefinition& material);
	};
}
