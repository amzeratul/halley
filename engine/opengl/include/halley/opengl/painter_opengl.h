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

		void setClip(Rect4i clip, Vector2i renderTargetSize, bool enable, bool isScreen) override;

	protected:
		void setVertices(MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices) override;
		void drawTriangles(size_t numIndices) override;
		void setViewPort(Rect4i rect, Vector2i renderTargetSize, bool isScreen) override;

	private:
		GLuint vbo = 0;
#ifdef WITH_OPENGL
		GLuint vao = 0;
#endif
		GLuint veo = 0;
		std::unique_ptr<GLUtils> glUtils;

		void setupVertexAttributes(MaterialDefinition& material);
	};
}
