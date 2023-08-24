#include "halley/graphics/sprite/particles.h"

#include "halley/maths/random.h"
#include "halley/support/logger.h"

using namespace Halley;

Particles::Particles()
	: rng(&Random::getGlobal())
{
}

Particles::Particles(const ConfigNode& node, Resources& resources)
	: rng(&Random::getGlobal())
{
	load(node, resources);
}

void Particles::load(const ConfigNode& node, Resources& resources)
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
	velScale = node["velScale"].asVector3f(Vector3f(1, 1, 1));
	minHeight = node["minHeight"].asOptional<float>();
	startHeight = node["startHeight"].asFloat(0);
	maxParticles = node["maxParticles"].asOptional<int>();
	burst = node["burst"].asOptional<int>();
}

ConfigNode Particles::toConfigNode() const
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
	result["velScale"] = velScale;
	result["minHeight"] = minHeight;
	result["startHeight"] = startHeight;
	result["maxParticles"] = maxParticles;
	result["burst"] = burst;

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
	position = Vector3f(pos);
}

void Particles::setPosition(Vector3f pos)
{
	position = pos;
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

	// Spawn new particles
	spawn(static_cast<size_t>(toSpawn), static_cast<float>(t));

	// Update particles
	updateParticles(static_cast<float>(t));

	// Remove dead particles
	for (size_t i = 0; i < nParticlesAlive; ) {
		if (!particles[i].alive) {
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
		initializeParticle(start + i, i * timeSlice);
	}
}

void Particles::initializeParticle(size_t index, float time)
{
	const auto startAzimuth = Angle1f::fromDegrees(rng->getFloat(azimuth));
	const auto startElevation = Angle1f::fromDegrees(rng->getFloat(altitude));
	
	auto& particle = particles[index];
	particle.alive = true;
	particle.time = time;
	particle.ttl = rng->getFloat(ttl);
	particle.pos = getSpawnPosition();
	particle.angle = rotateTowardsMovement ? startAzimuth : Angle1f();
	particle.scale = rng->getFloat(initialScale);
	
	particle.vel = Vector3f(rng->getFloat(speed), startAzimuth, startElevation);

	auto& sprite = sprites[index];
	if (isAnimated()) {
		auto& anim = animationPlayers[index];
		anim.update(0, sprite);
	} else if (!baseSprites.empty()) {
		sprite = rng->getRandomElement(baseSprites);
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
		} else {
			const bool stopped = stopTime > 0.00001f && particle.time + stopTime >= particle.ttl;
			const auto a = stopped ? Vector3f() : acceleration;
			particle.pos += (particle.vel * time + a * (0.5f * time * time)) * velScale;
			particle.vel += a * time;

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

			if (rotateTowardsMovement && particle.vel.squaredLength() > 0.001f) {
				particle.angle = particle.vel.xy().angle();
			}

			const float t = particle.time / particle.ttl;

			sprites[i]
				.setPosition(particle.pos.xy() + Vector2f(0, -particle.pos.z))
				.setRotation(particle.angle)
				.setScale(scaleCurve.evaluate(t) * particle.scale)
				.setColour(colourGradient.evaluate(t))
				.setCustom1(Vector4f(particle.pos.xy(), 0, 0));
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
	return position + Vector3f(pos, startHeight);
}

ConfigNode ConfigNodeSerializer<Particles>::serialize(const Particles& particles, const EntitySerializationContext& context)
{
	return particles.toConfigNode();
}

Particles ConfigNodeSerializer<Particles>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return Particles(node, *context.resources);
}

void ConfigNodeSerializer<Particles>::deserialize(const EntitySerializationContext& context, const ConfigNode& node, Particles& target)
{
	target.load(node, *context.resources);
}

