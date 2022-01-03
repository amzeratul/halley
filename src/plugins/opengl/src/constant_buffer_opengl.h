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
		void update(gsl::span<const gsl::byte> data) override;
		void bind(int bindPoint);

	private:
		GLBuffer buffer;
	};
}
