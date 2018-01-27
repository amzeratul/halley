#pragma once
#include "halley/core/graphics/painter.h"
#include "dx11_buffer.h"

namespace Halley
{
	class DX11Video;

	class DX11Painter : public Painter
	{
	public:
		explicit DX11Painter(DX11Video& video, Resources& resources);
		
		void clear(Colour colour) override;
		void setMaterialPass(const Material& material, int pass) override;
		void setMaterialData(const Material& material) override;

		void doStartRender() override;
		void doEndRender() override;

		void setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices, bool standardQuadsOnly) override;
		void drawTriangles(size_t numIndices) override;
		void setViewPort(Rect4i rect) override;
		void setClip(Rect4i clip, bool enable) override;

		void onUpdateProjection(Material& material) override;

	private:
		DX11Video& video;

		DX11Buffer vertexBuffer;
		DX11Buffer indexBuffer;
		ID3D11InputLayout* layout;
		ID3D11RasterizerState* rasterizer = nullptr;
	};
}
