#pragma once
#include "halley/core/graphics/shader.h"

namespace Halley
{
	class DX11Shader : public Shader
	{
	public:
		DX11Shader(const ShaderDefinition& definition);

		int getUniformLocation(const String& name, ShaderType stage) override;
		int getBlockLocation(const String& name, ShaderType stage) override;
	};
}
