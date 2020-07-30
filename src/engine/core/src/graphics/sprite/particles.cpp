#include "graphics/sprite/particles.h"

using namespace Halley;

void Particles::update(Time t)
{
	
}

gsl::span<Sprite> Particles::getSprites()
{
	return gsl::span<Sprite>(sprites).subspan(0, nSprites);
}

gsl::span<const Sprite> Particles::getSprites() const
{
	return gsl::span<const Sprite>(sprites).subspan(0, nSprites);
}

int Particles::getMask() const
{
	return mask;
}

int Particles::getLayer() const
{
	return layer;
}

