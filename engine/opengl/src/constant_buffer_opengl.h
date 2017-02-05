#pragma once
#include "halley/core/graphics/material/material.h"

namespace Halley
{
	class ConstantBufferOpenGL : public MaterialConstantBuffer
	{
	public:
		explicit ConstantBufferOpenGL(const Material& material);
		void update(const Material& material) override;
		void bind(int pass);

	private:
		const Material& material;
	};
}
