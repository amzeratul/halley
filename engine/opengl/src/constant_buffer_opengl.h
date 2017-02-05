#pragma once
#include "halley/core/graphics/material/material.h"

namespace Halley
{
	class ConstantBufferOpenGL : public MaterialConstantBuffer
	{
	public:
		explicit ConstantBufferOpenGL(const Material& material);
		void update(const Vector<MaterialParameter>& uniforms) override;
		void bind(int pass) override;

	private:
		const Material& material;
	};
}
