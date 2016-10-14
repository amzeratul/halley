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
		virtual void bind(int textureUnit) const = 0;
		virtual void load(const TextureDescriptor& descriptor) = 0;

		static std::shared_ptr<Texture> loadResource(ResourceLoader& loader);
		unsigned int getNativeId() const { return textureId; }

		Vector2i getSize() const { return size; }

	protected:
		unsigned int textureId = -1;
		Vector2i size;
	};
}
