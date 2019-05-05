#pragma once
#include <d3d11.h>
#include "halley/core/graphics/material/material_definition.h"
#undef min
#undef max

namespace Halley
{
	class DX11Video;

	class DX11RasterizerOptions
	{
	public:
		bool scissor = false;
		CullingMode culling = CullingMode::None;

		bool operator==(const DX11RasterizerOptions& other) const;
		bool operator!=(const DX11RasterizerOptions& other) const;
		bool operator<(const DX11RasterizerOptions& other) const;
	};

	class DX11Rasterizer {
	public:
		DX11Rasterizer(DX11Video& video, DX11RasterizerOptions options);
		~DX11Rasterizer();

		void bind(DX11Video& video);
		const DX11RasterizerOptions& getOptions() const { return options; }

	private:
		ID3D11RasterizerState* rasterizer = nullptr;
		DX11RasterizerOptions options;
	};
}
