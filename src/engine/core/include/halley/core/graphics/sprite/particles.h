#pragma once

#include "sprite.h"
#include <halley/time/halleytime.h>

#include "animation_player.h"

namespace Halley {
	class Random;
	class Animation;
	
	class Particles {
		struct Particle {
			Vector2f pos;
			Vector2f vel;
			Angle1f angle;
			float scale = 1;
			float time = 0;
			float ttl = 1;
			bool alive = true;
		};
		
	public:
		Particles();
		Particles(const ConfigNode& node, Resources& resources);

		ConfigNode toConfigNode() const;

		void setEnabled(bool enabled);
		bool isEnabled() const;
		void setSpawnRateMultiplier(float value);
		float getSpawnRateMultiplier() const;

		void setPosition(Vector2f pos);
		void setSpawnArea(Vector2f area);

		void setAngle(float newAngle);
		
		void update(Time t);

		void setSprites(std::vector<Sprite> sprites);
		void setAnimation(std::shared_ptr<const Animation> animation);

		bool isAnimated() const;
		bool isAlive() const;
		
		[[nodiscard]] gsl::span<Sprite> getSprites();
		[[nodiscard]] gsl::span<const Sprite> getSprites() const;

	private:
		Random* rng;
		std::shared_ptr<Material> material;

		bool enabled = true;
		bool firstUpdate = true;
		float spawnRateMultiplier = 1.0f;

		std::vector<Sprite> sprites;
		std::vector<Particle> particles;
		std::vector<AnimationPlayerLite> animationPlayers;
		
		size_t nParticlesAlive = 0;
		size_t nParticlesVisible = 0;
		float pendingSpawn = 0;

		float spawnRate = 100;
		Vector2f spawnArea;
		float ttl = 1;
		float ttlScatter = 0;
		float speed = 100;
		float speedScatter = 0;
		float speedDamp = 0;
		Vector2f acceleration;
		float angle = 0;
		float angleScatter = 0;
		float startScale = 1;
		float endScale = 1;
		float fadeInTime = 0;
		float fadeOutTime = 0;
		float stopTime = 0;
		float directionScatter = 0;
		bool rotateTowardsMovement = false;
		bool destroyWhenDone = false;
		std::optional<size_t> maxParticles;
		std::optional<size_t> burst;

		std::vector<Sprite> baseSprites;
		std::shared_ptr<const Animation> baseAnimation;
		Vector2f position;

		void start();
		void spawn(size_t n);
		void initializeParticle(size_t index);
		void updateParticles(float t);

		Vector2f getSpawnPosition() const;
	};

	class Resources;
	template<>
	class ConfigNodeSerializer<Particles> {
	public:
		ConfigNode serialize(const Particles& particles, const ConfigNodeSerializationContext& context);
		Particles deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node);
	};
}