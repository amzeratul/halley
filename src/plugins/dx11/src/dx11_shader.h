#pragma once
#include "halley/core/graphics/shader.h"

#include <d3d11.h>
#undef min
#undef max

namespace Halley
{
	class DX11Video;

	class DX11Shader final : public Shader
	{
	public:
		DX11Shader(DX11Video& video, const ShaderDefinition& definition);
		~DX11Shader();

		int getUniformLocation(const String& name, ShaderType stage) override;
		int getBlockLocation(const String& name, ShaderType stage) override;

		void bind(DX11Video& video);
		void setMaterialLayout(DX11Video& video, const std::vector<MaterialAttribute>& attributes);

	private:
		String name;
		ID3D11VertexShader* vertexShader = nullptr;
		ID3D11PixelShader* pixelShader = nullptr;
		ID3D11GeometryShader* geometryShader = nullptr;
		ID3D11InputLayout* layout = nullptr;
		Bytes vertexBlob;

		void loadShader(DX11Video& video, ShaderType type, const Bytes& bytes);
	};
}
