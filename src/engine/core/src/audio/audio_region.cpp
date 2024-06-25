#include "audio_region.h"

using namespace Halley;

AudioRegion::AudioRegion(AudioRegionId id)
	: id(id)
{
}

AudioRegionId AudioRegion::getId() const
{
	return id;
}

void AudioRegion::addNeighbour(AudioRegionId id, float attenuation, float lowPassHz)
{
	// TODO
}

void AudioRegion::removeNeighbour(AudioRegionId id)
{
	// TODO
}
