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
	baseSprite.setImage(resources, "whitebox.png").setSize(Vector2f(10, 0.5f)).setColour(Colour4f(1, 1, 1, 0.5f));
	spawnRate = 500;
}

ConfigNode Particles::toConfigNode() const
{
	ConfigNode::MapType result;
	// TODO
	return result;
}

void Particles::setPosition(Vector2f pos)
{
	position = pos;
}

void Particles::update(Time t)
{
	pendingSpawn += static_cast<float>(t * spawnRate);
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
}

gsl::span<Sprite> Particles::getSprites()
{
	return gsl::span<Sprite>(sprites).subspan(0, nParticlesAlive);
}

gsl::span<const Sprite> Particles::getSprites() const
{
	return gsl::span<const Sprite>(sprites).subspan(0, nParticlesAlive);
}

int Particles::getMask() const
{
	return mask;
}

int Particles::getLayer() const
{
	return layer;
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
	particle.ttl = 1;
	particle.pos = getSpawnPosition();
	particle.angle = Angle1f::fromDegrees(340);
	particle.vel = Vector2f(rng->getFloat(400, 500), particle.angle);

	auto& sprite = sprites[index];
	sprite = baseSprite;
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
	return position + Vector2f(rng->getFloat(-500, 500), rng->getFloat(-200, 200));
}

ConfigNode ConfigNodeSerializer<Particles>::serialize(const Particles& particles, ConfigNodeSerializationContext& context)
{
	return particles.toConfigNode();
}

Particles ConfigNodeSerializer<Particles>::deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return Particles(node, *context.resources);
}

