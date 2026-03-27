#pragma once
#include "halley/api/video_api.h"
#include "dx11_buffer.h"

namespace Halley
{
	class DX11Video;

	class DX11StructuredBuffer final : public MaterialStructuredBuffer
	{
	public:
		explicit DX11StructuredBuffer(DX11Video& video);

		void update(gsl::span<const std::byte> data, size_t elementStride) override;
		size_t getSize() const override { return curSize; }
		size_t getStride() const override { return curStride; }

		ID3D11ShaderResourceView* getSRV() const { return buffer.getSRV(); }

	private:
		DX11Buffer buffer;
		size_t curSize = 0;
		size_t curStride = 0;
	};
}
