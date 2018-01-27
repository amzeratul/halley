#include "dx11_buffer.h"
#include "halley/utils/utils.h"
#include "dx11_video.h"
using namespace Halley;

DX11Buffer::DX11Buffer(DX11Video& video, Type type)
	: video(video)
	, type(type)
{
}

DX11Buffer::~DX11Buffer()
{
	if (buffer) {
		buffer->Release();
		buffer = nullptr;
	}
}

void DX11Buffer::setData(gsl::span<const gsl::byte> data)
{
	if (size_t(data.size_bytes()) > curSize) {
		resize(size_t(data.size_bytes()));
	}

	D3D11_MAPPED_SUBRESOURCE ms;
	auto& devCon = video.getDeviceContext();
	devCon.Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	memcpy(ms.pData, data.data(), data.size_bytes());
	devCon.Unmap(buffer, 0);
}

ID3D11Buffer*& DX11Buffer::getBuffer()
{
	return buffer;
}

void DX11Buffer::resize(size_t requestedSize)
{
	size_t targetSize = std::max(size_t(16), nextPowerOf2(requestedSize));

	if (buffer) {
		buffer->Release();
		buffer = nullptr;
	}

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = UINT(targetSize);
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	switch (type) {
	case Type::Constant:
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		break;
	case Type::Index:
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		break;
	case Type::Vertex:
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		break;
	}

	HRESULT result = video.getDevice().CreateBuffer(&bd, nullptr, &buffer);
	if (result != S_OK) {
		throw Exception("Unable to create DX buffer with size " + toString(targetSize));
	}

	curSize = targetSize;
}
