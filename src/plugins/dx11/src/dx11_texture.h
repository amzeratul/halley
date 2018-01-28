#pragma once
#include "halley/core/graphics/texture.h"
#include <d3d11.h>
#undef min
#undef max

namespace Halley
{
	class DX11Video;

	class DX11Texture : public Texture
	{
	public:
		DX11Texture(DX11Video& video, Vector2i size);
		~DX11Texture();

		void load(TextureDescriptor&& descriptor) override;

		ID3D11Texture2D* getTexture() const;
		ID3D11ShaderResourceView* getShaderResourceView() const;

	private:
		DX11Video& video;
		ID3D11Texture2D* texture = nullptr;
		ID3D11ShaderResourceView* srv = nullptr;
	};
}
