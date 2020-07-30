#pragma once

#include "sprite.h"
#include <halley/time/halleytime.h>

namespace Halley {
	class Random;
	
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
		
		[[nodiscard]] gsl::span<Sprite> getSprites();
		[[nodiscard]] gsl::span<const Sprite> getSprites() const;
		[[nodiscard]] int getMask() const;
		[[nodiscard]] int getLayer() const;

	private:
		Random* rng;
		std::shared_ptr<Material> material;

		std::vector<Sprite> sprites;
		std::vector<Particle> particles;
		size_t nParticlesAlive = 0;
		
		int mask = 1;
		int layer = 0;
		float spawnRate = 10;
		float pendingSpawn = 0;

		Sprite baseSprite;
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
