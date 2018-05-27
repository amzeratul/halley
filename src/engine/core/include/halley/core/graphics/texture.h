#pragma once

#include "halley/resources/resource.h"
#include "halley/maths/vector2.h"
#include <memory>

namespace Halley
{
	class TextureDescriptor;
	class ResourceLoader;

	class Texture : public AsyncResource
	{
	public:
		Texture(Vector2i size);

		virtual void load(TextureDescriptor&& descriptor) = 0;

		static std::shared_ptr<Texture> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Texture; }

		Vector2i getSize() const { return size; }

	protected:
		Vector2i size;
	};
}
