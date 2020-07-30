#pragma once

#include "sprite.h"
#include <halley/time/halleytime.h>

namespace Halley {
	class Particles {
	public:
		void update(Time t);
		
		[[nodiscard]] gsl::span<Sprite> getSprites();
		[[nodiscard]] gsl::span<const Sprite> getSprites() const;
		[[nodiscard]] int getMask() const;
		[[nodiscard]] int getLayer() const;

	private:
		std::shared_ptr<Material> material;

		std::vector<Sprite> sprites;

		size_t nSprites = 0;
		int mask = 1;
		int layer = 0;
	};
}
