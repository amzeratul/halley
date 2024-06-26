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
	AudioFilterBiquad filter;
	if (neighbour.lowPassHz) {
		filter.setLowPass(*neighbour.lowPassHz);
	}

	neighbours.push_back(Neighbour{ neighbour, filter });
}

void AudioRegion::removeNeighbour(AudioRegionId id)
{
	std_ex::erase_if(neighbours, [=] (const Neighbour& n) { return n.props.id == id; });
}

const Vector<AudioRegion::Neighbour>& AudioRegion::getNeighbours() const
{
	return neighbours;
}

Vector<AudioRegion::Neighbour>& AudioRegion::getNeighbours()
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
