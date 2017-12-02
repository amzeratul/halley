#pragma once

#include "halley/resources/resource.h"
#include "halley/maths/vector2.h"
#include <memory>
#include "halley/maths/vector4.h"
#include <limits>

namespace Halley
{
	class TextureDescriptor;
	class ResourceLoader;

	class Texture : public AsyncResource
	{
	public:
		virtual void load(TextureDescriptor&& descriptor) = 0;

		static std::shared_ptr<Texture> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Texture; }
		unsigned int getNativeId() const { return textureId; }

		Vector2i getSize() const { return size; }

		Vector4s getSlices() const;

	protected:
		unsigned int textureId = std::numeric_limits<unsigned int>::max();
		Vector2i size;
		Vector4s slices;

		void computeSlice();
	};
}
