#include "audio_region_handle_impl.h"

#include "audio_engine.h"
#include "halley/audio/audio_facade.h"

using namespace Halley;

AudioRegionHandleImpl::AudioRegionHandleImpl(AudioFacade& facade, AudioRegionId id)
	: facade(facade)
	, id(id)
{
}

AudioRegionHandleImpl::~AudioRegionHandleImpl()
{
	facade.regionNames.erase(id);
	AudioEngine* engine = facade.engine.get();

	facade.enqueue([regId = id, engine] ()
	{
		engine->destroyRegion(regId);
	});
}

AudioRegionId AudioRegionHandleImpl::getId() const
{
	return id;
}

void AudioRegionHandleImpl::enqueue(std::function<void(AudioRegion& region)> f)
{
	auto id = this->id;
	AudioEngine* engine = facade.engine.get();
	facade.enqueue([id, engine, f = std::move(f)] () {
		if (auto* region = engine->getRegion(id)) {
			f(*region);
		}
	});
}

void AudioRegionHandleImpl::addNeighbour(AudioRegionNeighbour neighbour, String name)
{
	enqueue([neighbour = std::move(neighbour), name = std::move(name)](AudioRegion& region) mutable
	{
		region.addNeighbour(std::move(neighbour), std::move(name));
	});
}

void AudioRegionHandleImpl::removeNeighbour(AudioRegionId otherId)
{
	enqueue([=] (AudioRegion& region)
	{
		region.removeNeighbour(otherId);
	});
}
