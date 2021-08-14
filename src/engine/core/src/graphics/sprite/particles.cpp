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
	spawnRate = node["spawnRate"].asFloat(100);
	spawnArea = node["spawnArea"].asVector2f(Vector2f(0, 0));
	ttl = node["ttl"].asFloat(1.0f);
	ttlScatter = node["ttlScatter"].asFloat(0.0f);
	speed = node["speed"].asFloat(100.0f);
	speedScatter = node["speedScatter"].asFloat(0.0f);
	speedDamp = node["speedDamp"].asFloat(0.0f);
	acceleration = node["acceleration"].asVector2f(Vector2f());
	angle = node["angle"].asFloat(0.0f);
	angleScatter = node["angleScatter"].asFloat(0.0f);
	startScale = node["startScale"].asFloat(1.0f);
	endScale = node["endScale"].asFloat(1.0f);
	fadeInTime = node["fadeInTime"].asFloat(0.0f);
	fadeOutTime = node["fadeOutTime"].asFloat(0.0f);
	stopTime = node["stopTime"].asFloat(0.0f);
	directionScatter = node["directionScatter"].asFloat(0.0f);
	rotateTowardsMovement = node["rotateTowardsMovement"].asBool(false);
	destroyWhenDone = node["destroyWhenDone"].asBool(false);

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

	if (maxParticles) {
		result["maxParticles"] = static_cast<int>(maxParticles.value());
	}
	if (burst) {
		result["burst"] = static_cast<int>(burst.value());
	}

	return result;
}

void Particles::setEnabled(bool e)
{
	enabled = e;
}

bool Particles::isEnabled() const
{
	return enabled;
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
	position = pos;
}

void Particles::setSpawnArea(Vector2f area)
{
	spawnArea = area;
}

void Particles::setAngle(float newAngle)
{
	angle = newAngle;
}

void Particles::start()
{
	if (burst) {
		spawn(burst.value());
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
	spawn(static_cast<size_t>(toSpawn));

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

void Particles::setSprites(std::vector<Sprite> sprites)
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

void Particles::spawn(size_t n)
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
	
	for (size_t i = start; i < nParticlesAlive; ++i) {
		initializeParticle(i);
	}
}

void Particles::initializeParticle(size_t index)
{
	const auto startDirection = Angle1f::fromDegrees(rng->getFloat(angle - angleScatter, angle + angleScatter));
	
	auto& particle = particles[index];
	particle.alive = true;
	particle.time = 0;
	particle.ttl = rng->getFloat(ttl - ttlScatter, ttl + ttlScatter);
	particle.pos = getSpawnPosition();
	particle.angle = rotateTowardsMovement ? startDirection : Angle1f();
	particle.scale = startScale;
	particle.vel = Vector2f(rng->getFloat(speed - speedScatter, speed + speedScatter), startDirection);

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
			particle.vel += acceleration * time;
			
			if (stopTime > 0.00001f && particle.time + stopTime >= particle.ttl) {
				particle.vel = damp(particle.vel, Vector2f(), 10.0f, time);
			}
			
			if (speedDamp > 0.0001f) {
				particle.vel = damp(particle.vel, Vector2f(), speedDamp, time);
			}

			if (directionScatter > 0.00001f) {
				particle.vel = particle.vel.rotate(Angle1f::fromDegrees(rng->getFloat(-directionScatter * time, directionScatter * time)));
			}
			
			particle.pos += particle.vel * time;

			if (rotateTowardsMovement && particle.vel.squaredLength() > 0.001f) {
				particle.angle = particle.vel.angle();
			}

			particle.scale = lerp(startScale, endScale, particle.time / particle.ttl);

			if (fadeInTime > 0.000001f || fadeOutTime > 0.00001f) {
				const float alpha = clamp(std::min(particle.time / fadeInTime, (particle.ttl - particle.time) / fadeOutTime), 0.0f, 1.0f);
				sprites[i].getColour().a = alpha;
			}

			sprites[i]
				.setPosition(particle.pos)
				.setRotation(particle.angle)
				.setScale(particle.scale);
		}
	}
}

Vector2f Particles::getSpawnPosition() const
{
	return position + Vector2f(rng->getFloat(-spawnArea.x * 0.5f, spawnArea.x * 0.5f), rng->getFloat(-spawnArea.y * 0.5f, spawnArea.y * 0.5f));
}

ConfigNode ConfigNodeSerializer<Particles>::serialize(const Particles& particles, const ConfigNodeSerializationContext& context)
{
	return particles.toConfigNode();
}

Particles ConfigNodeSerializer<Particles>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return Particles(node, *context.resources);
}

