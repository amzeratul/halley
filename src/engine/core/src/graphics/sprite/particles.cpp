#include "halley/graphics/sprite/particles.h"

#include "halley/maths/polygon.h"
#include "halley/maths/random.h"
#include "halley/support/logger.h"

using namespace Halley;

Particles::Particles()
	: rng(&Random::getGlobal())
{
}

Particles::Particles(const ConfigNode& node, Resources& resources, const EntitySerializationContext& context)
	: rng(&Random::getGlobal())
{
	load(node, resources, context);
}

void Particles::load(const ConfigNode& node, Resources& resources, const EntitySerializationContext& context)
{
	spawnRate = node["spawnRate"].asFloat(100);
	spawnArea = node["spawnArea"].asVector2f(Vector2f(0, 0));
	spawnAreaShape = node["spawnAreaShape"].asEnum(ParticleSpawnAreaShape::Rectangle);

	if (node.hasKey("ttlScatter")) {
		// Legacy
		const auto legacyTtl = node["ttl"].asFloat(1.0f);
		const auto ttlScatter = node["ttlScatter"].asFloat(0.2f);
		ttl = Range<float>(legacyTtl - ttlScatter, legacyTtl + ttlScatter);
	} else {
		ttl = node["ttl"].asFloatRange(Range<float>(1.0f, 1.0f));
	}
	ttl.start = std::max(ttl.start, 0.1f);
	ttl.end = std::max(ttl.start, ttl.end);

	if (node.hasKey("speedScatter")) {
		// Legacy
		const auto legacySpeed = node["speed"].asFloat(100.0f);
		const auto speedScatter = node["speedScatter"].asFloat(0.0f);
		speed = Range<float>(legacySpeed - speedScatter, legacySpeed + speedScatter);
	} else {
		speed = node["speed"].asFloatRange(Range<float>(100.0f, 100.0f));
	}

	if (node.hasKey("angle")) {
		// Legacy
		const auto angle = node["angle"].asVector2f(Vector2f());
		const auto angleScatter = node["angleScatter"].asVector2f(Vector2f());
		azimuth = Range<float>(angle.x - angleScatter.x, angle.x + angleScatter.x);
		altitude = Range<float>(angle.y - angleScatter.y, angle.y + angleScatter.y);
	} else {
		azimuth = node["azimuth"].asFloatRange(Range<float>(0, 0));
		altitude = node["altitude"].asFloatRange(Range<float>(0, 0));
	}

	if (node.hasKey("startScale") || node.hasKey("endScale")) {
		const auto startScale = node["startScale"].asFloat(1.0f);
		const auto endScale = node["endScale"].asFloat(1.0f);
		const float scale = std::max(startScale, endScale);
		scaleCurve.makeDefault(false);
		scaleCurve.points[0].y = startScale / scale;
		scaleCurve.points[1].y = endScale / scale;
		scaleCurve.scale = scale;
	} else {
		scaleCurve = InterpolationCurve(node["scaleCurve"], false);
	}

	if (node.hasKey("fadeInTime") || node.hasKey("fadeOutTime")) {
		const auto fadeInTime = node["fadeInTime"].asFloat(0.0f);
		const auto fadeOutTime = node["fadeOutTime"].asFloat(0.0f);
		const float avgTTL = std::max((ttl.start + ttl.end) * 0.5f, 0.1f);
		colourGradient = ColourGradient(fadeInTime / avgTTL, 1.0f - fadeOutTime / avgTTL);
	} else {
		colourGradient = ColourGradient(node["colourGradient"]);
	}

	initialScale = node["initialScale"].asFloatRange(Range<float>(1.0f, 1.0f));
	speedDamp = node["speedDamp"].asFloat(0.0f);
	acceleration = node["acceleration"].asVector3f(Vector3f());
	stopTime = node["stopTime"].asFloat(0.0f);
	directionScatter = node["directionScatter"].asFloat(0.0f);
	rotateTowardsMovement = node["rotateTowardsMovement"].asBool(false);
	destroyWhenDone = node["destroyWhenDone"].asBool(false);
	relativePosition = node["relativePosition"].asBool(false);
	velScale = node["velScale"].asVector3f(Vector3f(1, 1, 1));
	minHeight = node["minHeight"].asOptional<float>();
	startHeight = node["startHeight"].asFloat(0);
	maxParticles = node["maxParticles"].asOptional<int>();
	burst = node["burst"].asOptional<int>();
	randomiseAnimationTime = node["randomiseAnimationTime"].asBool(false);
	onSpawn = ConfigNodeSerializer<EntityId>().deserialize(context, node["onSpawn"]);
	onDeath = ConfigNodeSerializer<EntityId>().deserialize(context, node["onDeath"]);

	maxBorder = {};
}

ConfigNode Particles::toConfigNode(const EntitySerializationContext& context) const
{
	ConfigNode::MapType result;

	result["spawnRate"] = spawnRate;
	result["spawnArea"] = spawnArea;
	result["spawnAreaShape"] = spawnAreaShape;
	result["ttl"] = ttl;
	result["speed"] = speed;
	result["speedDamp"] = speedDamp;
	result["acceleration"] = acceleration;
	result["azimuth"] = azimuth;
	result["altitude"] = altitude;
	result["initialScale"] = initialScale;
	result["scaleCurve"] = scaleCurve;
	result["colourGradient"] = colourGradient;
	result["stopTime"] = stopTime;
	result["directionScatter"] = directionScatter;
	result["rotateTowardsMovement"] = rotateTowardsMovement;
	result["destroyWhenDone"] = destroyWhenDone;
	result["relativePosition"] = relativePosition;
	result["velScale"] = velScale;
	result["minHeight"] = minHeight;
	result["startHeight"] = startHeight;
	result["maxParticles"] = maxParticles;
	result["burst"] = burst;
	result["randomiseAnimationTime"] = randomiseAnimationTime;
	result["onSpawn"] = ConfigNodeSerializer<EntityId>().serialize(onSpawn, context);
	result["onDeath"] = ConfigNodeSerializer<EntityId>().serialize(onDeath, context);

	return result;
}

void Particles::burstParticles(float n)
{
	pendingSpawn += n;
}

void Particles::reset()
{
	firstUpdate = true;
}

void Particles::setEnabled(bool e)
{
	enabled = e;
}

bool Particles::isEnabled() const
{
	return enabled;
}

void Particles::setSpawnRate(float value)
{
	spawnRate = value;
}

float Particles::getSpawnRate() const
{
	return spawnRate;
}

void Particles::setSpawnRateMultiplier(float value)
{
	spawnRateMultiplier = value;
}

float Particles::getSpawnRateMultiplier() const
{
	return spawnRateMultiplier;
}

void Particles::setPosition(Vector2f pos)
{
	setPosition(Vector3f(pos));
}

void Particles::setPosition(Vector3f pos)
{
	if (positionSet) {
		lastPosition = position;

		const auto delta = pos - position;
		if (delta.squaredLength() > 0.000001f) {
			if (relativePosition) {
				for (auto& particle: particles) {
					particle.pos += delta;
				}
			}

			position = pos;
		}
	} else {
		lastPosition = position = pos;
		positionSet = true;
	}
}

void Particles::setSpawnArea(Vector2f area)
{
	spawnArea = area;
}

Vector2f Particles::getSpawnArea() const
{
	return spawnArea;
}

void Particles::setSpawnAreaShape(ParticleSpawnAreaShape shape)
{
	spawnAreaShape = shape;
}

ParticleSpawnAreaShape Particles::getSpawnAreaShape() const
{
	return spawnAreaShape;
}

void Particles::setAzimuth(Range<float> azimuth)
{
	this->azimuth = azimuth;
}

void Particles::setAzimuth(float azimuth)
{
	setAzimuth(Range<float>(azimuth, azimuth));
}

void Particles::setAltitude(Range<float> altitudeAngle)
{
	this->altitude = altitudeAngle;
}

void Particles::setAltitude(float altitudeAngle)
{
	setAltitude(Range<float>(altitudeAngle, altitudeAngle));
}

Range<float> Particles::getAzimuth() const
{
	return azimuth;
}

Range<float> Particles::getAltitude() const
{
	return altitude;
}

void Particles::setSpeed(Range<float> speed)
{
	this->speed = speed;
}

void Particles::setSpeed(float speed)
{
	setSpeed(Range<float>(speed, speed));
}

Range<float> Particles::getSpeed() const
{
	return speed;
}

void Particles::setSpeedMultiplier(float value)
{
	speedMultiplier = value;
}

float Particles::getSpeedMultiplier() const
{
	return speedMultiplier;
}

void Particles::setAcceleration(Vector3f accel)
{
	acceleration = accel;
}

Vector3f Particles::getAcceleration() const
{
	return acceleration;
}

void Particles::setMinHeight(std::optional<float> z)
{
	minHeight = z;
}

std::optional<float> Particles::getMinHeight() const
{
	return minHeight;
}

void Particles::setSpawnHeight(float height)
{
	startHeight = height;
}

float Particles::getSpawnHeight() const
{
	return startHeight;
}

void Particles::setSpawnPositionOffset(Vector2f offset)
{
	spawnPositionOffset = offset;
}

void Particles::start()
{
	if (burst) {
		spawn(burst.value(), 0);
	}
	pendingSpawn = clamp(pendingSpawn, 0.0f, 1.0f);
}

void Particles::update(Time t)
{
	if (firstUpdate) {
		firstUpdate = false;
		start();
	}
	
	pendingSpawn += static_cast<float>(t * spawnRate * (enabled && !burst ? spawnRateMultiplier : 0));
	const int toSpawn = static_cast<int>(floor(pendingSpawn));
	pendingSpawn = pendingSpawn - static_cast<float>(toSpawn);

	if (toSpawn > 0) {
		// Spawn new particles
		spawn(static_cast<size_t>(toSpawn), static_cast<float>(t));
	}

	// Update particles
	updateParticles(static_cast<float>(t));

	// Update visibility
	nParticlesVisible = nParticlesAlive;
	if (nParticlesVisible > 0 && !sprites[0].hasMaterial()) {
		nParticlesVisible = 0;
	}
}

void Particles::setSprites(Vector<Sprite> sprites)
{
	baseSprites = std::move(sprites);
}

void Particles::setAnimation(std::shared_ptr<const Animation> animation)
{
	baseAnimation = std::move(animation);
}

bool Particles::isRandomisingAnimationTime() const
{
	return randomiseAnimationTime;
}

bool Particles::isAnimated() const
{
	return !!baseAnimation;
}

bool Particles::isAlive() const
{
	return nParticlesAlive > 0 || !destroyWhenDone;
}

gsl::span<Sprite> Particles::getSprites()
{
	return gsl::span<Sprite>(sprites).subspan(0, nParticlesVisible);
}

gsl::span<const Sprite> Particles::getSprites() const
{
	return gsl::span<const Sprite>(sprites).subspan(0, nParticlesVisible);
}

void Particles::setSecondarySpawner(IParticleSpawner* spawner)
{
	secondarySpawner = spawner;
}

void Particles::spawnAt(Vector3f pos)
{
	spawn(1, 0.0f);
	particles[nParticlesAlive - 1].pos = pos;
}

void Particles::destroyOverlapping(const Polygon& polygon)
{
	for (size_t i = 0; i < nParticlesAlive; ++i) {
		auto& particle = particles[i];
		if (polygon.isPointInside(particle.pos.xy())) {
			particle.alive = false;
		}
	}
}

void Particles::destroyOverlapping(const Ellipse& ellipse)
{
	for (size_t i = 0; i < nParticlesAlive; ++i) {
		auto& particle = particles[i];
		if (ellipse.contains(particle.pos.xy())) {
			particle.alive = false;
		}
	}
}

void Particles::destroyOverlapping(const Circle& circle)
{
	for (size_t i = 0; i < nParticlesAlive; ++i) {
		auto& particle = particles[i];
		if (circle.contains(particle.pos.xy())) {
			particle.alive = false;
		}
	}
}

void Particles::spawn(size_t n, float time)
{
	if (maxParticles) {
		n = std::min(n, maxParticles.value() - nParticlesAlive);
	}

	const size_t start = nParticlesAlive;
	nParticlesAlive += n;
	const size_t size = std::max(size_t(8), nextPowerOf2(nParticlesAlive));
	if (particles.size() < size) {
		particles.resize(size);
		sprites.resize(size);
	}
	if (animationPlayers.size() < size && isAnimated()) {
		animationPlayers.resize(size, AnimationPlayerLite(baseAnimation));
	}

	const float timeSlice = time / n;
	for (size_t i = 0; i < n; ++i) {
		initializeParticle(start + i, i * timeSlice, time);
	}
}

void Particles::initializeParticle(size_t index, float time, float totalTime)
{
	const auto startAzimuth = Angle1f::fromDegrees(rng->getFloat(azimuth));
	const auto startElevation = Angle1f::fromDegrees(rng->getFloat(altitude));
	
	auto& particle = particles[index];
	particle.firstFrame = true;
	particle.alive = true;
	particle.time = time;
	particle.ttl = rng->getFloat(ttl);
	//particle.angle = rotateTowardsMovement ? startAzimuth : Angle1f();
	particle.scale = rng->getFloat(initialScale);

	particle.vel = Vector3f(rng->getFloat(speed) * speedMultiplier, startAzimuth, startElevation);
	const bool stopped = stopTime > 0.00001f && particle.time + stopTime >= particle.ttl;
	const auto a = stopped ? Vector3f() : acceleration;
	const auto spawnPosSmear = totalTime > 0.00001f ? lerp(position - lastPosition, Vector3f(), time / totalTime) : Vector3f();
	particle.pos = getSpawnPosition() + spawnPosSmear + (particle.vel * time + a * (0.5f * time * time)) * velScale;

	auto& sprite = sprites[index];
	if (isAnimated()) {
		auto& anim = animationPlayers[index];
		anim.update(0, sprite);
		if (randomiseAnimationTime) {
			anim.update(Random::getGlobal().getFloat({0.0f, 42.0f}), sprite);
		}
	} else if (!baseSprites.empty()) {
		// Optimization: if there's only one baseSprite, and this sprite has a material, then we don't need to update it at all here
		if (!sprite.hasMaterial() || baseSprites.size() >= 2) {
			sprite.copyFrom(rng->getRandomElement(baseSprites), false);
		}
	}

	if (onSpawn) {
		onSecondarySpawn(particle, onSpawn);
	}
}

void Particles::updateParticles(float time)
{
	const bool hasAnim = isAnimated();

	for (size_t i = 0; i < nParticlesAlive; ++i) {
		if (hasAnim) {
			animationPlayers[i].update(time, sprites[i]);
		}

		auto& particle = particles[i];

		particle.time += time;
		if (particle.time >= particle.ttl) {
			particle.alive = false;
		}
		else {
			const bool stopped = stopTime > 0.00001f && particle.time + stopTime >= particle.ttl;
			const auto a = stopped ? Vector3f() : acceleration;
			if (particle.firstFrame) {
				particle.firstFrame = false;
			}
			else {
				particle.pos += (particle.vel * time + a * (0.5f * time * time)) * velScale;
				particle.vel += a * time;
			}

			if (minHeight && particle.pos.z < minHeight) {
				particle.alive = false;
			}

			if (stopped) {
				particle.vel = damp(particle.vel, Vector3f(), 10.0f, time);
			}

			if (speedDamp > 0.0001f) {
				particle.vel = damp(particle.vel, Vector3f(), speedDamp, time);
			}

			if (directionScatter > 0.00001f) {
				particle.vel = Vector3f(particle.vel.xy().rotate(Angle1f::fromDegrees(rng->getFloat(-directionScatter * time, directionScatter * time))), particle.vel.z);
			}
		}
	}

	removeDeadParticles();
}

void Particles::updateSprites(Time t)
{
	for (size_t i = 0; i < nParticlesAlive; ++i) {
		const auto& particle = particles[i];

		Angle1f angle;
		if (rotateTowardsMovement && particle.vel.squaredLength() > 0.001f) {
			angle = (particle.vel.xy() + Vector2f(0, particle.vel.z)).angle();
		}

		const float t = particle.time / particle.ttl;

		sprites[i]
			.setPosition(particle.pos.xy() + Vector2f(0, -particle.pos.z))
			.setRotation(angle)
			.setScale(scaleCurve.evaluate(t) * particle.scale)
			.setColour(colourGradient.evaluatePrecomputed(t))
			.setCustom1(Vector4f(particle.pos.xy(), 0, 0));
	}
}

void Particles::removeDeadParticles()
{
	for (size_t i = 0; i < nParticlesAlive; ) {
		if (!particles[i].alive) {
			if (onDeath) {
				onSecondarySpawn(particles[i], onDeath);
			}

			if (i != nParticlesAlive - 1) {
				// Swap with last particle that's alive
				std::swap(particles[i], particles[nParticlesAlive - 1]);
				std::swap(sprites[i], sprites[nParticlesAlive - 1]);
				if (isAnimated()) {
					std::swap(animationPlayers[i], animationPlayers[nParticlesAlive - 1]);
				}
			}
			--nParticlesAlive;
			// Don't increment i here, since i is now a new particle that's still alive
		} else {
			++i;
		}
	}
}

Vector3f Particles::getSpawnPosition() const
{
	Vector2f pos;
	if (spawnAreaShape == ParticleSpawnAreaShape::Rectangle) {
		pos = Vector2f(rng->getFloat(-1, 1), rng->getFloat(-1, 1)) * spawnArea * 0.5f;
	} else if (spawnAreaShape == ParticleSpawnAreaShape::Ellipse) {
		const float radius = std::sqrt(rng->getFloat(0, 1));
		const float angle = rng->getFloat(0.0f, 2.0f * pif());
		pos = Vector2f(radius, 0).rotate(Angle1f::fromRadians(angle)) * spawnArea * 0.5f;
	}
	return position + Vector3f(pos + spawnPositionOffset, startHeight);
}

void Particles::onSecondarySpawn(const Particle& particle, EntityId target)
{
	if (secondarySpawner && target) {
		secondarySpawner->spawn(particle.pos, target);
	}
}

std::optional<Rect4f> Particles::getAABB() const
{
	if (nParticlesAlive == 0) {
		return {};
	}

	Vector2f minPos = particles[0].pos.xy() + Vector2f(0, -particles[0].pos.z);
	Vector2f maxPos = minPos;

	for (size_t i = 1; i < nParticlesAlive; ++i) {
		const auto p = particles[i].pos.xy() + Vector2f(0, -particles[i].pos.z);
		minPos = Vector2f::min(minPos, p);
		maxPos = Vector2f::max(maxPos, p);
	}

	if (!maxBorder) {
		computeMaxBorder();
	}

	return Rect4f(minPos, maxPos).grow(*maxBorder);
}

float Particles::getSpriteBorder(const Sprite& sprite) const
{
	const auto topRight = (sprite.getSize() - sprite.getAbsolutePivot()).abs();
	const auto bottomLeft = sprite.getAbsolutePivot().abs();
	return Vector2f::max(topRight, bottomLeft).length();
}

void Particles::computeMaxBorder() const
{
	float biggestBorder = 0;
	if (baseAnimation) {
		const auto bounds = baseAnimation->getBounds();
		const auto delta = Vector2i::max(bounds.getBottomLeft().abs(), bounds.getTopRight().abs());
		biggestBorder = Vector2f(delta).length();
	} else {
		for (const auto& sprite: baseSprites) {
			biggestBorder = std::max(biggestBorder, getSpriteBorder(sprite));
		}
	}

	maxBorder = biggestBorder * scaleCurve.getMaxAbsValue();
}

ConfigNode ConfigNodeSerializer<Particles>::serialize(const Particles& particles, const EntitySerializationContext& context)
{
	return particles.toConfigNode(context);
}

Particles ConfigNodeSerializer<Particles>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return Particles(node, *context.resources, context);
}

void ConfigNodeSerializer<Particles>::deserialize(const EntitySerializationContext& context, const ConfigNode& node, Particles& target)
{
	target.load(node, *context.resources, context);
}

