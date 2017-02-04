#pragma once

#include "halley/resources/resource.h"
#include "halley/maths/vector2.h"
#include <memory>
#include "halley/maths/vector4.h"

namespace Halley
{
	class TextureDescriptor;
	class ResourceLoader;

	class Texture : public AsyncResource
	{
	public:
		virtual void bind(int textureUnit) const = 0;
		virtual void load(TextureDescriptor&& descriptor) = 0;

		static std::shared_ptr<Texture> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Texture; }
		unsigned int getNativeId() const { return textureId; }

		Vector2i getSize() const { return size; }

		Vector4s getSlices() const;

	protected:
		unsigned int textureId = -1;
		Vector2i size;
		Vector4s slices;

		void computeSlice();
	};
}
