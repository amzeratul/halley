#pragma once
#include "halley/graphics/texture.h"
#include <d3d11.h>

#include "halley/graphics/material/material_definition.h"

namespace Halley
{
	class DX11Video;

	class DX11Texture final : public Texture
	{
	public:
		DX11Texture(DX11Video& video, Vector2i size);
		DX11Texture(DX11Video& video, Vector2i size, ID3D11ShaderResourceView* srv);
		~DX11Texture();

		DX11Texture& operator=(DX11Texture&& other) noexcept;

		void doLoad(TextureDescriptor& descriptor) override;
		void reload(Resource&& resource) override;
		void bind(DX11Video& video, int textureUnit, TextureSamplerType samplerType) const;
		void generateMipMaps() override;

		DXGI_FORMAT getFormat() const;
		ID3D11Texture2D* getTexture() const;

		void replaceShaderResourceView(ID3D11ShaderResourceView* view);

#ifdef DEV_BUILD
		void setAssetId(String name) override;
#endif

	protected:
		size_t getVRamUsage() const override;
		void doCopyToTexture(Painter& painter, Texture& other) const override;
		void doCopyToImage(Painter& painter, Image& image) const override;

	private:
		DX11Video& video;
		ID3D11Texture2D* texture = nullptr;
		ID3D11ShaderResourceView* srv = nullptr;
		ID3D11ShaderResourceView* srvAlt = nullptr;
		ID3D11SamplerState* samplerState = nullptr;
		DXGI_FORMAT format;
		size_t vramUsage = 0;

#ifdef DEV_BUILD
		std::optional<String> debugName;
#endif

		void copyToImageDirectly(Image& image) const;
		void clear();
	};
}
