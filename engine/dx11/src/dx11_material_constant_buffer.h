#pragma once
#include "halley/core/graphics/material/material.h"

namespace Halley
{
	class DX11MaterialConstantBuffer : public MaterialConstantBuffer
	{
	public:
		void update(const MaterialDataBlock& dataBlock) override;
	};
}
