#include "dx11_shader.h"
using namespace Halley;

DX11Shader::DX11Shader(const ShaderDefinition& definition)
{
}

int DX11Shader::getUniformLocation(const String& name, ShaderType stage)
{
	return -1;
}

int DX11Shader::getBlockLocation(const String& name, ShaderType stage)
{
	return -1;
}
