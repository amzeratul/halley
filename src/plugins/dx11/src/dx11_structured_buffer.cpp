#include "dx11_structured_buffer.h"
#include "dx11_video.h"
using namespace Halley;

DX11StructuredBuffer::DX11StructuredBuffer(DX11Video& video)
	: buffer(video, DX11Buffer::Type::Structured)
{
}

void DX11StructuredBuffer::update(gsl::span<const std::byte> data, size_t elementStride)
{
	buffer.reset();
	buffer.setData(data, elementStride);
	curSize = data.size_bytes();
	curStride = elementStride;
}
