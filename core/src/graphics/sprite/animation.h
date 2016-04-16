#pragma once

namespace Halley
{
	class ResourceLoader;
	class SpriteSheet;
	class Material;

	class Animation : public Resource
	{
		friend class AnimationPlayer;

	public:
		static std::unique_ptr<Animation> loadResource(ResourceLoader& loader);

	private:
		explicit Animation(ResourceLoader& loader);

		String name;
		std::shared_ptr<SpriteSheet> spriteSheet;
		std::shared_ptr<Material> material;
	};
}
