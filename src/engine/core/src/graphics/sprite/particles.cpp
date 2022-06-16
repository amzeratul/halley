#include "graphics/sprite/particles.h"

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
	ttl = node["ttl"].asFloat(1.0f);
	ttlScatter = node["ttlScatter"].asFloat(0.0f);
	speed = node["speed"].asFloat(100.0f);
	speedScatter = node["speedScatter"].asFloat(0.0f);
	speedDamp = node["speedDamp"].asFloat(0.0f);
	acceleration = node["acceleration"].asVector3f(Vector3f());
	angle = node["angle"].asVector2f(Vector2f());
	angleScatter = node["angleScatter"].asVector2f(Vector2f());
	startScale = node["startScale"].asFloat(1.0f);
	endScale = node["endScale"].asFloat(1.0f);
	fadeInTime = node["fadeInTime"].asFloat(0.0f);
	fadeOutTime = node["fadeOutTime"].asFloat(0.0f);
	stopTime = node["stopTime"].asFloat(0.0f);
	directionScatter = node["directionScatter"].asFloat(0.0f);
	rotateTowardsMovement = node["rotateTowardsMovement"].asBool(false);
	destroyWhenDone = node["destroyWhenDone"].asBool(false);
	velScale = node["velScale"].asVector3f(Vector3f(1, 1, 1));
	if (node.hasKey("minHeight")) {
		minHeight = node["minHeight"].asFloat();
	}
	startHeight = node["startHeight"].asFloat(0);

	if (node.hasKey("maxParticles")) {
		maxParticles = node["maxParticles"].asInt();
	}
	if (node.hasKey("burst")) {
		burst = node["burst"].asInt();
	}
}

ConfigNode Particles::toConfigNode() const
{
	ConfigNode::MapType result;

	result["spawnRate"] = spawnRate;
	result["spawnArea"] = spawnArea;
	result["ttl"] = ttl;
	result["ttlScatter"] = ttlScatter;
	result["speed"] = speed;
	result["speedScatter"] = speedScatter;
	result["speedDamp"] = speedDamp;
	result["acceleration"] = acceleration;
	result["angle"] = angle;
	result["angleScatter"] = angleScatter;
	result["startScale"] = startScale;
	result["endScale"] = endScale;
	result["fadeInTime"] = fadeInTime;
	result["fadeOutTime"] = fadeOutTime;
	result["stopTime"] = stopTime;
	result["directionScatter"] = directionScatter;
	result["rotateTowardsMovement"] = rotateTowardsMovement;
	result["destroyWhenDone"] = destroyWhenDone;
	result["velScale"] = velScale;
	if (minHeight) {
		result["minHeight"] = *minHeight;
	}
	result["startHeight"] = startHeight;

	if (maxParticles) {
		result["maxParticles"] = static_cast<int>(maxParticles.value());
	}
	if (burst) {
		result["burst"] = static_cast<int>(burst.value());
	}

	return result;
}

void Particles::burstParticles(float n)
{
	pendingSpawn += n;
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

void Particles::setAngle(float newAngle)
{
	angle = Vector2f(newAngle, 0);
}

void Particles::setAngle(Vector2f newAngle)
{
	angle = newAngle;
}

Vector2f Particles::getAngle() const
{
	return angle;
}

void Particles::setSpeed(float newSpeed)
{
	speed = newSpeed;
}

float Particles::getSpeed() const
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
		if (isAnimated()) {
			animationPlayers.resize(size, AnimationPlayerLite(baseAnimation));
		}
	}

	const float timeSlice = time / n;
	for (size_t i = 0; i < n; ++i) {
		initializeParticle(start + i, i * timeSlice);
	}
}

void Particles::initializeParticle(size_t index, float time)
{
	const auto startAzimuth = Angle1f::fromDegrees(rng->getFloat(angle.x - angleScatter.x, angle.x + angleScatter.x));
	const auto startElevation = Angle1f::fromDegrees(rng->getFloat(angle.y - angleScatter.y, angle.y + angleScatter.y));
	
	auto& particle = particles[index];
	particle.alive = true;
	particle.time = time;
	particle.ttl = rng->getFloat(ttl - ttlScatter, ttl + ttlScatter);
	particle.pos = getSpawnPosition();
	particle.angle = rotateTowardsMovement ? startAzimuth : Angle1f();
	particle.scale = startScale;
	
	particle.vel = Vector3f(rng->getFloat(speed - speedScatter, speed + speedScatter), startAzimuth, startElevation);

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
			particle.pos += (particle.vel * time + acceleration * (0.5f * time * time)) * velScale;
			particle.vel += acceleration * time;

			if (minHeight && particle.pos.z < minHeight) {
				particle.alive = false;
			}
			
			if (stopTime > 0.00001f && particle.time + stopTime >= particle.ttl) {
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

			particle.scale = lerp(startScale, endScale, particle.time / particle.ttl);

			if (fadeInTime > 0.000001f || fadeOutTime > 0.00001f) {
				const float alpha = clamp(std::min(particle.time / fadeInTime, (particle.ttl - particle.time) / fadeOutTime), 0.0f, 1.0f);
				sprites[i].getColour().a = alpha;
			}

			sprites[i]
				.setPosition(particle.pos.xy() + Vector2f(0, -particle.pos.z))
				.setRotation(particle.angle)
				.setScale(particle.scale);
		}
	}
}

Vector3f Particles::getSpawnPosition() const
{
	return position + Vector3f(rng->getFloat(-spawnArea.x * 0.5f, spawnArea.x * 0.5f), rng->getFloat(-spawnArea.y * 0.5f, spawnArea.y * 0.5f), startHeight);
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

