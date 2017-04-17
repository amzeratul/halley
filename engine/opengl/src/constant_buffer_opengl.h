#pragma once
#include "halley/core/graphics/material/material.h"
#include "gl_buffer.h"

namespace Halley
{
	class ConstantBufferOpenGL : public MaterialConstantBuffer
	{
	public:
		explicit ConstantBufferOpenGL();
		~ConstantBufferOpenGL();
		void update(const MaterialDataBlock& dataBlock) override;
		void bind(int bindPoint);

	private:
		GLBuffer buffer;
	};
}
