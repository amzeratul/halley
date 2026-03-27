#pragma once
#include "halley_dx12.h"
#include "halley/api/video_api.h"
#include "dx12_resource.h"

namespace Halley
{
	class DX12Video;

	class DX12StructuredBuffer final : public MaterialStructuredBuffer
	{
	public:
		explicit DX12StructuredBuffer(DX12Video& video);

		void update(gsl::span<const std::byte> data, size_t elementStride) override;
		size_t getSize() const override { return curSize; }
		size_t getStride() const override { return curStride; }

		DX12Buffer& getBuffer() { return buffer; }
		D3D12_GPU_VIRTUAL_ADDRESS getGPUVirtualAddress() const;

	private:
		DX12Buffer buffer;
		size_t curSize = 0;
		size_t curStride = 0;
	};
}
