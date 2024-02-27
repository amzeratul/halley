#pragma once

#include "sprite.h"
#include "halley/time/halleytime.h"
#include "halley/maths/interpolation_curve.h"
#include "animation_player.h"
#include "halley/entity/entity_id.h"
#include "halley/maths/circle.h"
#include "halley/maths/colour_gradient.h"
#include "halley/maths/ellipse.h"

namespace Halley {
	class Polygon;
	class Random;
	class Animation;

	enum class ParticleSpawnAreaShape : uint8_t {
		Rectangle,
		Ellipse
	};

	template <>
	struct EnumNames<ParticleSpawnAreaShape> {
		constexpr std::array<const char*, 2> operator()() const {
			return{{
				"rectangle",
				"ellipse"
			}};
		}
	};

	class IParticleSpawner {
	public:
		virtual ~IParticleSpawner() = default;
		virtual void spawn(Vector3f pos, EntityId target) = 0;
	};
	
	class Particles {
		struct Particle {
			Vector3f pos;
			Vector3f vel;
			Angle1f angle;
			float scale = 1;
			float time = 0;
			float ttl = 1;
			bool alive = true;
			bool firstFrame = true;
		};
		
	public:
		Particles();
		Particles(const ConfigNode& node, Resources& resources, const EntitySerializationContext& context);
		void load(const ConfigNode& node, Resources& resources, const EntitySerializationContext& context);

		ConfigNode toConfigNode(const EntitySerializationContext& context) const;

		void burstParticles(float n);

		void reset();

		void setEnabled(bool enabled);
		bool isEnabled() const;

		void setSpawnRate(float value);
		float getSpawnRate() const;
		void setSpawnRateMultiplier(float value);
		float getSpawnRateMultiplier() const;

		void setPosition(Vector2f pos);
		void setPosition(Vector3f pos);
		void setSpawnArea(Vector2f area);
		Vector2f getSpawnArea() const;
		void setSpawnAreaShape(ParticleSpawnAreaShape shape);
		ParticleSpawnAreaShape getSpawnAreaShape() const;

		void setAzimuth(Range<float> azimuth);
		void setAzimuth(float azimuth);
		void setAltitude(Range<float> altitudeAngle);
		void setAltitude(float altitudeAngle);
		Range<float> getAzimuth() const;
		Range<float> getAltitude() const;

		void setSpeed(Range<float> speed);
		void setSpeed(float speed);
		Range<float> getSpeed() const;
		void setSpeedMultiplier(float value);
		float getSpeedMultiplier() const;

		void setAcceleration(Vector3f acceleration);
		Vector3f getAcceleration() const;

		void setMinHeight(std::optional<float> minZ);
		std::optional<float> getMinHeight() const;
		void setSpawnHeight(float height);
		float getSpawnHeight() const;
		void setSpawnPositionOffset(Vector2f offset);

		void update(Time t);

		void setSprites(Vector<Sprite> sprites);
		void setAnimation(std::shared_ptr<const Animation> animation);

		bool isAnimated() const;
		bool isAlive() const;
		
		[[nodiscard]] gsl::span<Sprite> getSprites();
		[[nodiscard]] gsl::span<const Sprite> getSprites() const;

		void setSecondarySpawner(IParticleSpawner* spawner);
		void spawnAt(Vector3f pos);

		Rect4f getAABB() const;
		void destroyOverlapping(const Polygon& polygon);
		void destroyOverlapping(const Ellipse& ellipse);
		void destroyOverlapping(const Circle& circle);

	private:
		Random* rng;
		std::shared_ptr<Material> material;

		bool enabled = true;
		bool firstUpdate = true;
		float spawnRateMultiplier = 1.0f;
		float speedMultiplier = 1.0f;

		Vector<Sprite> sprites;
		Vector<Particle> particles;
		Vector<AnimationPlayerLite> animationPlayers;
		
		size_t nParticlesAlive = 0;
		size_t nParticlesVisible = 0;
		float pendingSpawn = 0;

		float spawnRate = 100;
		Vector2f spawnArea;
		ParticleSpawnAreaShape spawnAreaShape = ParticleSpawnAreaShape::Rectangle;
		float startHeight = 0;
		Vector2f spawnPositionOffset;

		Range<float> ttl;
		Range<float> speed;
		Range<float> azimuth;
		Range<float> altitude;
		Range<float> initialScale;
		float speedDamp = 0;
		Vector3f acceleration;
		Vector3f velScale;
		InterpolationCurve scaleCurve;
		ColourGradient colourGradient;
		float stopTime = 0;
		float directionScatter = 0;
		bool rotateTowardsMovement = false;
		bool destroyWhenDone = false;
		bool positionSet = false;
		bool relativePosition = false;
		std::optional<int> maxParticles;
		std::optional<int> burst;
		std::optional<float> minHeight;

		Vector<Sprite> baseSprites;
		std::shared_ptr<const Animation> baseAnimation;
		Vector3f position;
		Vector3f lastPosition;
		EntityId onSpawn;
		EntityId onDeath;

		IParticleSpawner* secondarySpawner = nullptr;

		void start();
		void initializeParticle(size_t index, float time, float totalTime);
		void updateParticles(float t);
		void spawn(size_t n, float time);

		Vector3f getSpawnPosition() const;

		void onSecondarySpawn(const Particle& particle, EntityId target);
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
