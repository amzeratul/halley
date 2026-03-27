#include "dx12_structured_buffer.h"
#include "dx12_video.h"
using namespace Halley;

DX12StructuredBuffer::DX12StructuredBuffer(DX12Video& video)
	: buffer(video, DX12Buffer::Type::Constant, 0)
{
}

void DX12StructuredBuffer::update(gsl::span<const std::byte> data, size_t elementStride)
{
	const size_t size = data.size_bytes();
	buffer.resize(size);

	auto dest = buffer.map();
	gsl::copy(data, dest);
	buffer.unmap(0, size);

	curSize = size;
	curStride = elementStride;
}

D3D12_GPU_VIRTUAL_ADDRESS DX12StructuredBuffer::getGPUVirtualAddress() const
{
	auto* resource = buffer.getResource();
	return resource ? resource->GetGPUVirtualAddress() : 0;
}
