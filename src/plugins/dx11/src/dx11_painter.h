#pragma once
#include "halley/core/graphics/painter.h"
#include "dx11_buffer.h"
#include <map>

#include "dx11_rasterizer.h"
#include "dx11_blend.h"
#include "dx11_depth_stencil.h"

namespace Halley
{
	class MaterialPass;
	class DX11Video;
	class DX11Blend;
	class DX11Rasterizer;
	class DX11RasterizerOptions;
	class DX11DepthStencil;

	class DX11Painter final : public Painter
	{
	public:
		explicit DX11Painter(DX11Video& video, Resources& resources);
		
		void clear(std::optional<Colour> colour, std::optional<float> depth, std::optional<uint8_t> stencil) override;
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
		ID3D11InputLayout* layout = nullptr;
		std::map<BlendType, DX11Blend> blendModes;

		std::map<DX11RasterizerOptions, std::unique_ptr<DX11Rasterizer>> rasterizers;
		DX11Rasterizer* curRaster = nullptr;

		std::unordered_map<MaterialDepthStencil, std::unique_ptr<DX11DepthStencil>> depthStencils;
		DX11DepthStencil* curDepthStencil = nullptr;

		size_t curBuffer = 0;
		std::optional<Rect4i> clipping;
		std::vector<int> renderTargetTextureUnits;

		DX11Blend& getBlendMode(BlendType type);
		void rotateBuffers();

		DX11Rasterizer& getRasterizer(const DX11RasterizerOptions& options);
		void setRasterizer(const MaterialPass& pass);
		void setRasterizer(const DX11RasterizerOptions& options);

		DX11DepthStencil& getDepthStencil(const MaterialDepthStencil& depthStencil);
		void setDepthStencil(const MaterialDepthStencil& depthStencil);

		void unbindRenderTargetTextureUnits(size_t lastIndex, int minimumTextureUnit);
	};
}
