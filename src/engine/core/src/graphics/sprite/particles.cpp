#include "graphics/sprite/particles.h"

#include "halley/maths/random.h"

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
	angle = node["angle"].asFloat(0.0f);
	angleScatter = node["angleScatter"].asFloat(0.0f);
}

ConfigNode Particles::toConfigNode() const
{
	ConfigNode::MapType result;
	// TODO
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

void Particles::setPosition(Vector2f pos)
{
	position = pos;
}

void Particles::update(Time t)
{
	if (enabled) {
		pendingSpawn += static_cast<float>(t * spawnRate);
	}
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
	const size_t start = nParticlesAlive;
	nParticlesAlive += n;
	const size_t size = std::max(size_t(8), nextPowerOf2(nParticlesAlive));
	if (particles.size() < size) {
		particles.resize(size);
		sprites.resize(size);
	}
	
	for (size_t i = start; i < nParticlesAlive; ++i) {
		initializeParticle(i);
	}
}

void Particles::initializeParticle(size_t index)
{	
	auto& particle = particles[index];
	particle.alive = true;
	particle.ttl = rng->getFloat(ttl - ttlScatter, ttl + ttlScatter);
	particle.pos = getSpawnPosition();
	particle.angle = Angle1f::fromDegrees(rng->getFloat(angle - angleScatter, angle + angleScatter));
	particle.vel = Vector2f(rng->getFloat(speed - speedScatter, speed + speedScatter), particle.angle);

	auto& sprite = sprites[index];
	if (!baseSprites.empty()) {
		sprite = rng->getRandomElement(baseSprites);
	}
}

void Particles::updateParticles(float time)
{
	for (size_t i = 0; i < nParticlesAlive; ++i) {
		auto& particle = particles[i];

		particle.ttl -= time;
		if (particle.ttl <= 0) {
			particle.alive = false;
		} else {
			particle.pos += particle.vel * time;
			particle.angle = particle.vel.angle();
			
			sprites[i]
				.setPosition(particle.pos)
				.setRotation(particle.angle);
		}
	}
}

Vector2f Particles::getSpawnPosition() const
{
	return position + Vector2f(rng->getFloat(-spawnArea.x * 0.5f, spawnArea.x * 0.5f), rng->getFloat(-spawnArea.y * 0.5f, spawnArea.y * 0.5f));
}

ConfigNode ConfigNodeSerializer<Particles>::serialize(const Particles& particles, ConfigNodeSerializationContext& context)
{
	return particles.toConfigNode();
}

Particles ConfigNodeSerializer<Particles>::deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return Particles(node, *context.resources);
}

