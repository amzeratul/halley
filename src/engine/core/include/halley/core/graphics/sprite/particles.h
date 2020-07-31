#pragma once

#include "sprite.h"
#include <halley/time/halleytime.h>

namespace Halley {
	class Random;
	class Animation;
	
	class Particles {
		struct Particle {
			Vector2f pos;
			Vector2f vel;
			Angle1f angle;
			float ttl = 1;
			bool alive = true;
		};
		
	public:
		Particles();
		Particles(const ConfigNode& node, Resources& resources);

		ConfigNode toConfigNode() const;

		void setPosition(Vector2f pos);
		void update(Time t);

		void setSprites(const std::vector<Sprite>& sprites);
		void setAnimation(std::shared_ptr<const Animation> animation);
		
		[[nodiscard]] gsl::span<Sprite> getSprites();
		[[nodiscard]] gsl::span<const Sprite> getSprites() const;

	private:
		Random* rng;
		std::shared_ptr<Material> material;

		std::vector<Sprite> sprites;
		std::vector<Particle> particles;
		size_t nParticlesAlive = 0;
		size_t nParticlesVisible = 0;
		float pendingSpawn = 0;
		
		float spawnRate = 100;
		Vector2f spawnArea;
		float ttl = 1;
		float ttlScatter = 0;
		float speed = 100;
		float speedScatter = 0;
		float angle = 0;
		float angleScatter = 0;

		Sprite baseSprite;
		std::shared_ptr<const Animation> baseAnimation;
		Vector2f position;

		void spawn(size_t n);
		void initializeParticle(size_t index);
		void updateParticles(float t);

		Vector2f getSpawnPosition() const;
	};

	class Resources;
	template<>
	class ConfigNodeSerializer<Particles> {
	public:
		ConfigNode serialize(const Particles& particles, ConfigNodeSerializationContext& context);
		Particles deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node);
	};
}
