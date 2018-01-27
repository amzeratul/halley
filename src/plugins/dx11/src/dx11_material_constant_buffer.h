#pragma once
#include "halley/core/graphics/material/material.h"
#include "dx11_buffer.h"

namespace Halley
{
	class DX11MaterialConstantBuffer : public MaterialConstantBuffer
	{
	public:
		DX11MaterialConstantBuffer(DX11Video& video);

		void update(const MaterialDataBlock& dataBlock) override;
		DX11Buffer& getBuffer();

	private:
		DX11Buffer buffer;
	};
}
