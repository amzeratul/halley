#include "dx11_material_constant_buffer.h"

#include "dx11_video.h"
#include "halley/graphics/shader_type.h"
using namespace Halley;

DX11MaterialConstantBuffer::DX11MaterialConstantBuffer(DX11Video& video)
	: buffer(video, DX11Buffer::Type::Constant)
{
}

void DX11MaterialConstantBuffer::update(gsl::span<const gsl::byte> data)
{
	buffer.reset();
	buffer.setData(data);
}

DX11Buffer& DX11MaterialConstantBuffer::getBuffer()
{
	return buffer;
}

DX11ShaderStorageBuffer::DX11ShaderStorageBuffer(DX11Video& video)
	: video(video)
	, buffer(video, DX11Buffer::Type::StructuredBuffer)
{
}

DX11ShaderStorageBuffer::~DX11ShaderStorageBuffer()
{
	clearView();
}

void DX11ShaderStorageBuffer::update(size_t numElements, size_t pitch, gsl::span<const gsl::byte> data)
{
	buffer.reset();
	buffer.setData(data, pitch);

	if (numSlots < numElements) {
		size_t elementsAllocated = numElements;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = static_cast<UINT>(elementsAllocated);
		auto result = video.getDevice().CreateShaderResourceView(buffer.getBuffer(), &srvDesc, &srv);

		if (FAILED(result)) {
			throw Exception("Failed to create ShaderStorageBuffer", HalleyExceptions::Graphics);
		}

		numSlots = elementsAllocated;
	}
}

void DX11ShaderStorageBuffer::bind(ShaderType type, int position)
{
	if (type == ShaderType::Vertex) {
		video.getDeviceContext().VSSetShaderResources(position, 1, &srv);
	} else if (type == ShaderType::Pixel) {
		video.getDeviceContext().PSSetShaderResources(position, 1, &srv);
	}
}

void DX11ShaderStorageBuffer::clearView()
{
	if (srv) {
		srv->Release();
		srv = nullptr;
	}
	numSlots = 0;
}
