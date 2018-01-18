#pragma once
#include "halley/core/graphics/texture.h"

namespace Halley
{
	class DX11Texture : public Texture
	{
	public:
		DX11Texture(Vector2i size);
		void load(TextureDescriptor&& descriptor) override;
	};
}
