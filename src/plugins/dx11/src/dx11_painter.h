#pragma once
#include "halley/core/graphics/painter.h"
#include "dx11_buffer.h"
#include <map>

namespace Halley
{
	class MaterialPass;
	class DX11Video;
	class DX11Blend;
	class DX11Rasterizer;
	class DX11RasterizerOptions;
	class DX11DepthStencil;

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

		std::vector<DX11Buffer> vertexBuffers;
		std::vector<DX11Buffer> indexBuffers;
		ID3D11InputLayout* layout;
		std::map<BlendType, DX11Blend> blendModes;

		std::map<DX11RasterizerOptions, std::unique_ptr<DX11Rasterizer>> rasterizers;
		DX11Rasterizer* curRaster = nullptr;

		std::unique_ptr<DX11DepthStencil> depthStencil;

		size_t curBuffer = 0;
		std::optional<Rect4i> clipping;

		DX11Blend& getBlendMode(BlendType type);
		void rotateBuffers();

		DX11Rasterizer& getRasterizer(const DX11RasterizerOptions& options);
		void setRasterizer(const MaterialPass& pass);
		void setRasterizer(const DX11RasterizerOptions& options);

		void setDepthStencil(const MaterialPass& pass);
	};
}
