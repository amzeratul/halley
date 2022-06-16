#pragma once

#include "sprite.h"
#include <halley/time/halleytime.h>

#include "animation_player.h"

namespace Halley {
	class Random;
	class Animation;
	
	class Particles {
		struct Particle {
			Vector3f pos;
			Vector3f vel;
			Angle1f angle;
			float scale = 1;
			float time = 0;
			float ttl = 1;
			bool alive = true;
		};
		
	public:
		Particles();
		Particles(const ConfigNode& node, Resources& resources);
		void load(const ConfigNode& node, Resources& resources);

		ConfigNode toConfigNode() const;

		void burstParticles(size_t n);

		void setEnabled(bool enabled);
		bool isEnabled() const;
		void setSpawnRateMultiplier(float value);
		float getSpawnRateMultiplier() const;

		void setPosition(Vector2f pos);
		void setPosition(Vector3f pos);
		void setSpawnArea(Vector2f area);

		void setAngle(float newAngle);
		void setAngle(Vector2f newAngle);
		Vector2f getAngle() const;

		void setSpeed(float speed);
		float getSpeed() const;
		void setAcceleration(Vector3f acceleration);
		Vector3f getAcceleration() const;

		void setMinHeight(std::optional<float> minZ);
		std::optional<float> getMinHeight() const;
		void setSpawnHeight(float height);
		float getSpawnHeight() const;
		
		void update(Time t);

		void setSprites(Vector<Sprite> sprites);
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

		Vector<Sprite> sprites;
		Vector<Particle> particles;
		Vector<AnimationPlayerLite> animationPlayers;
		
		size_t nParticlesAlive = 0;
		size_t nParticlesVisible = 0;
		float pendingSpawn = 0;

		float spawnRate = 100;
		Vector2f spawnArea;
		float startHeight = 0;
		float ttl = 1;
		float ttlScatter = 0;
		float speed = 100;
		float speedScatter = 0;
		float speedDamp = 0;
		Vector3f acceleration;
		Vector3f velScale;
		Vector2f angle;
		Vector2f angleScatter;
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
		std::optional<float> minHeight;

		Vector<Sprite> baseSprites;
		std::shared_ptr<const Animation> baseAnimation;
		Vector3f position;

		void start();
		void initializeParticle(size_t index);
		void updateParticles(float t);
		void spawn(size_t n);

		Vector3f getSpawnPosition() const;
	};

	class Resources;
	template<>
	class ConfigNodeSerializer<Particles> {
	public:
		ConfigNode serialize(const Particles& particles, const EntitySerializationContext& context);
		Particles deserialize(const EntitySerializationContext& context, const ConfigNode& node);
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, Particles& target);
	};
}