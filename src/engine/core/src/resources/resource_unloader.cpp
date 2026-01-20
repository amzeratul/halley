#include "halley/resources/resource_unloader.h"

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

ResourceUnloader::ResourceUnloader(Resources& resources)
	: resources(resources)
{
}

void ResourceUnloader::update(Time t, const ResourceUnloaderRules& rules)
{
	for (auto& [type, rule]: rules.rules) {
		updateCollection(t, resources.ofType(type), rule);
	}
}

void ResourceUnloader::updateCollection(Time t, ResourceCollectionBase& collection, const ResourceUnloaderAssetTypeRules& rules)
{
	if (!collection.isAsync()) {
		return;
	}

	const bool verbose = false;

	HashMap<ResourceDesiredLoadState, StateCollection> states;
	updateResourcesAndCollectStates(t, collection, states);
	
	size_t startMemoryUsage = 0;
	for (auto& [type, stateCollection]: states) {
		stateCollection.sort();
		startMemoryUsage += stateCollection.curMemoryUsage;
	}
	size_t curMemoryUsage = startMemoryUsage;

	// If we've decided to unload those, do it
	for (auto& s: states[ResourceDesiredLoadState::Unload].states) {
		if (s.loaded) {
			s.markAsUnloading = true;
			curMemoryUsage -= s.memoryUsage;
		}
	}

	// Mark all "Loaded" that aren't actually loaded as loading
	for (auto& s: states[ResourceDesiredLoadState::Load].states) {
		if (!s.loaded) {
			s.markAsLoading = true;
			curMemoryUsage += s.memoryUsage;
		}
	}
	
	// Check preloads
	size_t memoryUsageWithAllPreloads = curMemoryUsage;
	for (auto& s: states[ResourceDesiredLoadState::Preload].states) {
		if (!s.loaded) {
			memoryUsageWithAllPreloads += s.memoryUsage;
		}
	}
	for (auto& s: states[ResourceDesiredLoadState::PreloadLowPriority].states) {
		if (!s.loaded) {
			memoryUsageWithAllPreloads += s.memoryUsage;
		}
	}

	// Stale, might unload
	for (auto& s: states[ResourceDesiredLoadState::Stale].states) {
		if (s.loaded && (memoryUsageWithAllPreloads > rules.budget || curMemoryUsage > rules.staleBudget)) {
			s.markAsUnloading = true;
			memoryUsageWithAllPreloads -= s.memoryUsage;
			curMemoryUsage -= s.memoryUsage;
		}
	}

	// Preload
	for (auto& s: states[ResourceDesiredLoadState::Preload].states) {
		if (!s.loaded && curMemoryUsage + s.memoryUsage <= rules.budget) {
			s.markAsLoading = true;
			curMemoryUsage += s.memoryUsage;
		}
	}
	for (auto& s: states[ResourceDesiredLoadState::PreloadLowPriority].states) {
		if (!s.loaded && curMemoryUsage + s.memoryUsage <= rules.budget) {
			s.markAsLoading = true;
			curMemoryUsage += s.memoryUsage;
		}
	}

	if (curMemoryUsage > rules.budget) {
		Logger::logError("Memory budget for " + toString(collection.getAssetType()) + " exceeded: "
			+ String::prettySize(curMemoryUsage) + "/" + String::prettySize(rules.budget));
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

void ResourceUnloader::updateResourcesAndCollectStates(Time t, ResourceCollectionBase& collection, HashMap<ResourceDesiredLoadState, StateCollection>& states)
{
	collection.forEachResource([&] (const std::shared_ptr<Resource>& resource) {
		auto res = std::dynamic_pointer_cast<AsyncResource>(resource);

		res->startFrame(t);

		const bool unloadable = res->canUnload();
		const auto usagePattern = res->getUsagePattern();
		const auto memoryUsage = usagePattern.loaded ? res->getMemoryUsage() : res->getEstimatedMemoryUsage();

		LoadStateInfo state;
		state.loaded = usagePattern.loaded;
		state.memoryUsage = memoryUsage.getTotal();
		state.timeSinceUse = std::min(usagePattern.timeSinceInUse, usagePattern.timeSinceInBackground * 2.0); // Time spent in background counts as double

		if (!unloadable || usagePattern.framesSinceInUse <= 1 || usagePattern.timeSinceInUse < 0.1) {
			state.desiredState = ResourceDesiredLoadState::Load;
		} else if (usagePattern.framesSinceInBackground <= 1 || usagePattern.timeSinceInBackground < 1) {
			state.desiredState = ResourceDesiredLoadState::Preload;
		} else if (usagePattern.framesSinceInLowPriorityBackground <= 1 || usagePattern.timeSinceInLowPriorityBackground < 1) {
			state.desiredState = ResourceDesiredLoadState::PreloadLowPriority;
		} else if (usagePattern.timeSinceInUse < 15 || usagePattern.timeSinceInBackground < 10) {
			state.desiredState = ResourceDesiredLoadState::Stale;
		} else {
			state.desiredState = ResourceDesiredLoadState::Unload;
		}
		res->setDesiredLoadState(state.desiredState);

		state.res = std::move(res);

		auto& stateCollection = states[state.desiredState];
		if (usagePattern.loaded) {
			stateCollection.curMemoryUsage += memoryUsage.getTotal();
		}
		stateCollection.states += std::move(state);
	});
}
