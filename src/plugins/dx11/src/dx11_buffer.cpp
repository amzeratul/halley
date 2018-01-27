#include "dx11_buffer.h"
#include "halley/utils/utils.h"
#include "dx11_video.h"
using namespace Halley;

DX11Buffer::DX11Buffer(DX11Video& video)
	: video(video)
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
		resize(nextPowerOf2(size_t(data.size_bytes())));
	}

	D3D11_MAPPED_SUBRESOURCE ms;
	auto& devCon = video.getDeviceContext();
	devCon.Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	memcpy(ms.pData, data.data(), data.size_bytes());
	devCon.Unmap(buffer, 0);
}

void DX11Buffer::resize(size_t size)
{
	if (buffer) {
		buffer->Release();
		buffer = nullptr;
	}

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = size;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT result = video.getDevice().CreateBuffer(&bd, nullptr, &buffer);
	if (result != S_OK) {
		throw Exception("Unable to create DX buffer with size " + toString(size));
	}

	curSize = size;
}
