#include "dx11_material_constant_buffer.h"
using namespace Halley;

DX11MaterialConstantBuffer::DX11MaterialConstantBuffer(DX11Video& video)
	: buffer(video, DX11Buffer::Type::Constant)
{
}

void DX11MaterialConstantBuffer::update(const MaterialDataBlock& dataBlock)
{
	buffer.reset();
	buffer.setData(dataBlock.getData());
}

DX11Buffer& DX11MaterialConstantBuffer::getBuffer()
{
	return buffer;
}
