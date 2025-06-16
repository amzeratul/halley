#include "audio_region.h"

#include "audio_engine.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

AudioRegion::AudioRegion(AudioRegionId id, String name)
	: id(id)
	, name(std::move(name))
{
}

AudioRegionId AudioRegion::getId() const
{
	return id;
}

const String& AudioRegion::getName() const
{
	return name;
}

void AudioRegion::addNeighbour(AudioRegionNeighbour neighbour, String name)
{
	AudioFilterBiquad filter;
	if (neighbour.lowPassHz) {
		filter.setLowPass(*neighbour.lowPassHz);
	}

	neighbours.push_back(Neighbour{ neighbour, filter, std::move(name) });
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

void AudioRegion::incRefCount(AudioEngine& engine)
{
	if (refCount++ == 0) {
		for (const auto& n: neighbours) {
			if (auto* other = engine.getRegion(n.props.id)) {
				other->incRefCount(engine);
			}
		}
	}
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
