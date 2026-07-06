#include "halley/resources/resource_unloader.h"

#include "halley/api/system_api.h"
#include "halley/graphics/texture.h"

using namespace Halley;

ResourceUnloaderAssetTypeRules::ResourceUnloaderAssetTypeRules(size_t budget)
	: budget(budget)
	, staleBudget((budget / 4) * 3)
{
}

ResourceUnloaderAssetTypeRules::ResourceUnloaderAssetTypeRules(size_t budget, size_t staleBudget)
	: budget(budget)
	, staleBudget(staleBudget)
{	
}

bool ResourceUnloader::LoadStateInfo::operator<(const LoadStateInfo& other) const
{
	if (desiredState != other.desiredState) {
		return desiredState > other.desiredState;
	}

	// Oldest first
	return timeSinceUse > other.timeSinceUse;
}

void ResourceUnloader::StateCollection::sort()
{
	std::sort(states.begin(), states.end());
}

ResourceUnloader::ResourceUnloader(Resources& resources, SystemAPI& systemAPI)
	: resources(resources)
	, systemAPI(systemAPI)
	, executor(SingleThreadExecutor("resourceUnloader", [&systemAPI] (String name, std::function<void()> f) {
		return systemAPI.createThread(name, ThreadPriority::Low, std::move(f));
	}))
{
}

void ResourceUnloader::update(Time origDt, const ResourceUnloaderRules& rules)
{
	if (pendingUpdate.isValid()) {
		pendingUpdate.wait();
	}
	
	const float dt = static_cast<float>(origDt);

	for (const auto& [type, rule]: rules.rules) {
		prepareCollection(dt, resources.ofType(type));
	}

	pendingUpdate = Concurrent::execute(executor.getQueue(), [this, dt, rules = rules] {
		for (auto& t: frameTimes) {
			t += dt;
		}
		while (!frameTimes.empty() && frameTimes[0] > maxTimeLogged) {
			frameTimes.erase(frameTimes.begin());
		}
		frameTimes.push_back(0);

		for (const auto& [type, rule]: rules.rules) {
			updateCollection(dt, resources.ofType(type), rule);
		}

		++frameIdx;
	});
}

void ResourceUnloader::prepareCollection(float dt, ResourceCollectionBase& collection)
{
	if (!collection.isAsync()) {
		return;
	}

	collection.forEachResource([&] (const std::shared_ptr<Resource>& resource) {
		resource->startFrame(dt, frameIdx);
	});
}

void ResourceUnloader::updateCollection(float t, ResourceCollectionBase& collection, const ResourceUnloaderAssetTypeRules& rules)
{
	if (!collection.isAsync()) {
		return;
	}

	if (budgetMessageTimeout > 0) {
		budgetMessageTimeout -= t;
	}
	if (unloadPreloadMessageTimeout > 0) {
		unloadPreloadMessageTimeout -= t;
	}

	const bool verbose = false;

	auto& states = lastStates;
	for (auto& [k, v]: states) {
		v.curMemoryUsage = 0;
		v.states.clear();
	}
	updateResourcesAndCollectStates(t, collection, states);
	
	size_t startMemoryUsage = 0;
	for (auto& [type, stateCollection]: states) {
		stateCollection.sort();
		startMemoryUsage += stateCollection.curMemoryUsage;
	}
	size_t curMemoryUsage = startMemoryUsage;

	size_t unloadableKept = 0;

	// Anything marked as "Unload" will be unloaded
	for (auto& s: states[ResourceDesiredLoadState::Unload].states) {
		if (s.loaded) {
			s.markAsUnloading = true;
			curMemoryUsage -= s.memoryUsage;
		}
	}

	// Anything marked as "Load" will be loaded
	int nWaitingToLoad = 0;
	for (auto& s: states[ResourceDesiredLoadState::Load].states) {
		if (!s.loaded) {
			++nWaitingToLoad;
			s.markAsLoading = true;
			curMemoryUsage += s.memoryUsage;
		}
	}
	const bool canPreload = nWaitingToLoad == 0;
	
	// Figure out how much more memory we'd need if we wanted to load all preloads
	size_t memoryUsageWithAllPreloads = curMemoryUsage;
	if (canPreload) {
		for (const auto state: { ResourceDesiredLoadState::Preload, ResourceDesiredLoadState::PreloadLowPriority }) {
			for (auto& s: states[state].states) {
				if (!s.loaded) {
					memoryUsageWithAllPreloads += s.memoryUsage;
				}
			}
		}
	}

	// Stale, unload if it makes space for preloads, or if we're above the "stale budget" (normally ~75% of total budget, so we have some headroom)
	// Otherwise it's OK to keep them around
	for (auto& s: states[ResourceDesiredLoadState::Stale].states) {
		if (s.loaded && (memoryUsageWithAllPreloads > rules.budget || curMemoryUsage > rules.staleBudget)) {
			s.markAsUnloading = true;
			memoryUsageWithAllPreloads -= s.memoryUsage;
			curMemoryUsage -= s.memoryUsage;
		} else {
			++unloadableKept;
		}
	}

	// Unload preloaded if we're still out of memory (this is not ideal)
	bool preloadUnloaded = false;
	for (const auto state: { ResourceDesiredLoadState::PreloadLowPriority, ResourceDesiredLoadState::Preload }) {
		for (auto& s: states[state].states) {
			if (s.loaded && curMemoryUsage > rules.budget) {
				s.markAsUnloading = true;
				curMemoryUsage -= s.memoryUsage;
				preloadUnloaded = true;
			} else {
				++unloadableKept;
			}
		}
	}

	// Preload
	if (canPreload) {
		for (const auto state: { ResourceDesiredLoadState::Preload, ResourceDesiredLoadState::PreloadLowPriority }) {
			bool preloadingAny = false;
			for (auto& s: states[state].states) {
				if (!s.loaded && curMemoryUsage + s.memoryUsage <= rules.budget) {
					s.markAsLoading = true;
					curMemoryUsage += s.memoryUsage;
					preloadingAny = true;
				}
			}
			if (preloadingAny) { // Don't proceed to the next category if we had to preload anything on the current category
				break;
			}
		}
	}

	// If we had to unload any preloads, notify warning
	if (preloadUnloaded) {
		if (unloadPreloadMessageTimeout <= 0.001) {
			Logger::logWarning("Memory budget for " + toString(collection.getAssetType()) + " under pressure: "
				+ String::prettySize(curMemoryUsage) + "/" + String::prettySize(rules.budget) + " - preloaded resources had to be unloaded");
			unloadPreloadMessageTimeout = 1.0;
		}
	}

	// If we've run out of memory and there was nothing else we could have unloaded, notify error
	if (curMemoryUsage > static_cast<size_t>(rules.budget * 1.1f) && unloadableKept > 0) {
		if (budgetMessageTimeout <= 0.001) {
			Logger::logError("Memory budget for " + toString(collection.getAssetType()) + " exceeded: "
				+ String::prettySize(curMemoryUsage) + "/" + String::prettySize(rules.budget));
			budgetMessageTimeout = 1.0;
		}
	}

	// Do unloads
	for (auto& [type, stateCol]: states) {
		for (auto& state: stateCol.states) {
			if (state.markAsUnloading) {
				const bool unloaded = state.res->requestUnloading();
				if (unloaded && verbose) {
					Logger::logDev(String("Unloaded [") + toString(collection.getAssetType()) + "] " + state.res->getAssetId());
				}
			}
		}
	}

	// Do loads, in this specific order
	for (auto& type: { ResourceDesiredLoadState::Load, ResourceDesiredLoadState::Preload, ResourceDesiredLoadState::PreloadLowPriority }) {
		const auto iter = states.find(type);
		if (iter == states.end()) {
			continue;
		}

		for (auto& state: iter->second.states) {
			if (state.markAsLoading) {
				const bool loading = state.res->requestLoading();
				if (loading && verbose) {
					if (state.desiredState == ResourceDesiredLoadState::Load) {
						Logger::logDev(String("Loading [") + toString(collection.getAssetType()) + "] " + state.res->getAssetId());
					} else if (state.desiredState == ResourceDesiredLoadState::Preload) {
						Logger::logDev(String("Preloading [") + toString(collection.getAssetType()) + "] " + state.res->getAssetId());
					} else if (state.desiredState == ResourceDesiredLoadState::PreloadLowPriority) {
						Logger::logDev(String("Preloading Low-Priority [") + toString(collection.getAssetType()) + "] " + state.res->getAssetId());
					}
				}
			}
		}
	}
}

void ResourceUnloader::updateResourcesAndCollectStates(float t, ResourceCollectionBase& collection, HashMap<ResourceDesiredLoadState, StateCollection>& states)
{
	//Logger::logDev("Updating " + toString(collection.enumerate().size()) + " resources.");
	auto resources = collection.getAllResources();

	for (auto& resource: resources) {
		LoadStateInfo state;
		if (collection.getAssetType() == AssetType::Texture) {
			state = getStateInfo<Texture>(std::static_pointer_cast<Texture>(std::move(resource)), t);
		} else {
			state = getStateInfo<AsyncResource>(std::static_pointer_cast<AsyncResource>(std::move(resource)), t);
		}

		auto& stateCollection = states[state.desiredState];
		if (state.loaded) {
			stateCollection.curMemoryUsage += state.memoryUsage;
		}
		stateCollection.states += std::move(state);
	};
}

float ResourceUnloader::getTimeSince(uint32_t idx) const
{
	const auto framesAgo = getFramesSince(idx);
	const auto maxIdx = frameTimes.size() - 1;
	if (framesAgo > maxIdx) {
		return maxTimeLogged + 0.1f;
	}

	return frameTimes[maxIdx - framesAgo];
}

uint32_t ResourceUnloader::getFramesSince(uint32_t idx) const
{
	if (idx > frameIdx) {
		return 0;
	}
	return frameIdx - idx;
}

template <typename T>
ResourceUnloader::LoadStateInfo ResourceUnloader::getStateInfo(std::shared_ptr<T> res, float t) const
{
	const AsyncResource::UsagePattern usagePattern = res->getUsagePattern();
	const auto loaded = res->isLoaded();
	const ResourceMemoryUsage memoryUsage = loaded ? res->getMemoryUsage() : res->getEstimatedMemoryUsage();

	const uint32_t framesSinceInUse = getFramesSince(usagePattern.lastFrameInUse);
	const uint32_t framesSinceInBackground = getFramesSince(usagePattern.lastFrameInBackground);
	const uint32_t framesSinceInLowPriorityBackground = getFramesSince(usagePattern.lastFrameInBackgroundLowPriority);
	const float timeSinceInUse = getTimeSince(usagePattern.lastFrameInUse);
	const float timeSinceInBackground = getTimeSince(usagePattern.lastFrameInBackground);
	const float timeSinceInLowPriorityBackground = getTimeSince(usagePattern.lastFrameInBackgroundLowPriority);

	LoadStateInfo state;
	state.loaded = loaded;
	state.memoryUsage = memoryUsage.getTotal();
	state.timeSinceUse = std::min(timeSinceInUse, timeSinceInBackground * 2.0f); // Time spent in background counts as double

	if (framesSinceInUse <= 1 || timeSinceInUse < 0.1 || !res->canUnload()) {
		state.desiredState = ResourceDesiredLoadState::Load;
	} else if (framesSinceInBackground <= 1 || timeSinceInBackground < 1) {
		state.desiredState = ResourceDesiredLoadState::Preload;
	} else if (framesSinceInLowPriorityBackground <= 1 || timeSinceInLowPriorityBackground < 1) {
		state.desiredState = ResourceDesiredLoadState::PreloadLowPriority;
	} else if (timeSinceInUse < 15 || timeSinceInBackground < 10) {
		state.desiredState = ResourceDesiredLoadState::Stale;
	} else {
		state.desiredState = ResourceDesiredLoadState::Unload;
	}
	res->setDesiredLoadState(state.desiredState);

	state.res = std::move(res);
	return state;
}
