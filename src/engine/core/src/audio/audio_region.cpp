#include "audio_region.h"

#include "halley/utils/algorithm.h"

using namespace Halley;

AudioRegion::AudioRegion(AudioRegionId id)
	: id(id)
{
}

AudioRegionId AudioRegion::getId() const
{
	return id;
}

void AudioRegion::addNeighbour(AudioRegionNeighbour neighbour)
{
	neighbours.push_back(neighbour);
}

void AudioRegion::removeNeighbour(AudioRegionId id)
{
	std_ex::erase_if(neighbours, [=] (const AudioRegionNeighbour& n) { return n.id == id; });
}

const Vector<AudioRegionNeighbour>& AudioRegion::getNeighbours() const
{
	return neighbours;
}

void AudioRegion::markAsReadyToDestroy()
{
	readyToDestroy = true;
}

void AudioRegion::clearRefCount()
{
	refCount = 0;
}

void AudioRegion::incRefCount()
{
	++refCount;
}

bool AudioRegion::shouldDestroy() const
{
	return readyToDestroy && refCount == 0;
}

void AudioRegion::setPrevGain(float gain)
{
	prevGain = gain;
}

float AudioRegion::getPrevGain() const
{
	return prevGain;
}
