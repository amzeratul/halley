#pragma once

#include "halley/core/graphics/painter.h"
#include "halley_gl.h"
#include "gl_buffer.h"

namespace Halley
{
	class Resources;
	class MaterialPass;

	class PainterOpenGL final : public Painter
	{
	public:
		PainterOpenGL(Resources& resources);
		~PainterOpenGL();

		void doStartRender() override;
		void doEndRender() override;

		void clear(std::optional<Colour> colour, std::optional<float> depth, std::optional<uint32_t> stencil) override;
		void setMaterialPass(const Material& material, int pass) override;
		void setMaterialData(const Material& material) override;

		void setClip(Rect4i clip, bool enable) override;

	protected:
		void setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices, bool standardQuadsOnly) override;
		void drawTriangles(size_t numIndices) override;
		void setViewPort(Rect4i rect) override;
		void onUpdateProjection(Material& material) override;

	private:
#ifdef WITH_OPENGL
		GLuint vao = 0;
#endif
		GLBuffer vertexBuffer;
		GLBuffer elementBuffer;
		GLBuffer stdQuadElementBuffer;
		std::unique_ptr<GLUtils> glUtils;

		void setupVertexAttributes(const MaterialDefinition& material);
	};
}
