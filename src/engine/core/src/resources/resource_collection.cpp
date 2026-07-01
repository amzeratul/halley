#include "halley/resources/resource_collection.h"
#include "halley/resources/resource_locator.h"
#include "halley/resources/resources.h"
#include <halley/resources/resource.h>
#include <utility>

#include "halley/graphics/sprite/sprite.h"
#include "halley/support/logger.h"
#include "halley/utils/scoped_guard.h"

using namespace Halley;


ResourceCollectionBase::ResourceCollectionBase(Resources& parent, AssetType type)
	: parent(parent)
	, type(type)
{
	HalleyAssertDev(!Debug::isRunningFromDLL());
}

void ResourceCollectionBase::clear()
{
	UniqueLock lock(mutex);

	resources.clear();
	resourceMap.clear();
}

void ResourceCollectionBase::unload(std::string_view assetId)
{
	UniqueLock lock(mutex);

	if (const auto iter = resourceMap.find(assetId); iter != resourceMap.end()) {
		const uint32_t idx = iter->second;
		resources[idx].res = {};
		resourceMap.erase(iter);
		freeIdxs += idx;
	}
}

void ResourceCollectionBase::unloadAll()
{
	clear();
}

void ResourceCollectionBase::reload(std::string_view assetId)
{
	SharedLock lock(mutex);
	
	const auto res = resourceMap.find(assetId);
	if (res != resourceMap.end()) {
		const auto idx = res->second;
		auto& oldAsset = *resources[idx].res;
		try {
			const auto [newAsset, loaded] = loadAsset(assetId, ResourceLoadPriority::High, false);
			newAsset->setAssetId(assetId);
			newAsset->onLoaded(parent);

			const auto oldIdx = oldAsset.getAssetIdx();
			oldAsset.reloadResource(std::move(*newAsset));
			oldAsset.setAssetIdx(oldIdx);
		} catch (std::exception& e) {
			Logger::logError("Error while reloading " + String(assetId) + ": " + e.what());
		} catch (...) {
			Logger::logError("Unknown error while reloading " + String(assetId));
		}
	}
}

void ResourceCollectionBase::purge(std::string_view assetId)
{
	if (!resourceLoader) {
		parent.locator->purge(assetId, type);
	}
}

std::shared_ptr<Resource> ResourceCollectionBase::getUntyped(std::string_view name, ResourceLoadPriority priority)
{
#ifdef VIRTUAL_RESOURCE_GET
	return get(name, priority, true);
#else
	return doGet(name, priority, true);
#endif
}

Vector<String> ResourceCollectionBase::enumerate() const
{
	if (resourceEnumerator) {
		return resourceEnumerator();
	} else {
		return parent.locator->enumerate(type);
	}
}

AssetType ResourceCollectionBase::getAssetType() const
{
	return type;
}

ResourceMemoryUsage ResourceCollectionBase::getMemoryUsage() const
{
	ResourceMemoryUsage usage;
	SharedLock lock(mutex);

	for (auto& r: resources) {
		if (r.res) {
			usage += r.res->getMemoryUsage();
		}
	}

	return usage;
}

void ResourceCollectionBase::generateDetailedMemoryReport(std::optional<int> limit) const
{
	struct Info {
		String assetId;
		ResourceMemoryUsage usage;
		int useCount = 0;
	};
	HashMap<ResourceDesiredLoadState, Vector<Info>> usages;

	{
		SharedLock lock(mutex);
		for (auto& r: resources) {
			if (auto& res = r.res) {
				const auto desiredState = isAsync() ?
					dynamic_cast<AsyncResource&>(*res).getDesiredLoadState() :
					ResourceDesiredLoadState::Undefined;
				usages[desiredState].emplace_back(Info{ res->getAssetId(), res->getMemoryUsage(), static_cast<int>(res.use_count()) });
			}
		}
	}

	for (auto& [loadState, entries]: usages) {
		std::sort(entries.begin(), entries.end(), [] (const Info& a, const Info& b) {
			return b.usage < a.usage;
		});
	}

	Logger::logInfo("Detailed memory report for " + toString(getAssetType()) + ":");

	const int maxN = limit.value_or(std::numeric_limits<int>::max());

	int i = 0;
	for (auto loadState: {
		ResourceDesiredLoadState::Load,
		ResourceDesiredLoadState::Preload,
		ResourceDesiredLoadState::PreloadLowPriority,
		ResourceDesiredLoadState::Stale,
		ResourceDesiredLoadState::Unload,
		ResourceDesiredLoadState::Undefined
	}) {
		const auto iter = usages.find(loadState);
		if (iter == usages.end()) {
			continue;
		}

		const auto& entries = iter->second;
		if (!entries.empty()) {
			size_t totalUsage = 0;
			for (auto& entry: entries) {
				totalUsage += entry.usage.getTotal();
			}
			Logger::logInfo("  " + toString(loadState) + " (" + String::prettySize(totalUsage) + "):");

			for (auto& entry: entries) {
				Logger::logInfo("    " + toString(++i, 10, 3, ' ') + ": "
					+ entry.assetId + " [" + entry.useCount + "]: "
					+ String::prettySize(entry.usage.getTotal()));

				if (i >= maxN) {
					return;
				}
			}
		}
	}
}

void ResourceCollectionBase::notifyResourcesUnloaded()
{
	SharedLock lock(mutex);

	for (auto& r: resources) {
		if (r.res) {
			r.res->onOtherResourcesUnloaded();
		}
	}
}

std::pair<std::shared_ptr<Resource>, bool> ResourceCollectionBase::loadAsset(std::string_view assetId, ResourceLoadPriority priority, bool allowFallback)
{
	HalleyAssertDev(!Debug::isRunningFromDLL());

	const auto trace = StackDebugTrace("assetId", assetId);

	std::shared_ptr<Resource> newRes;

	if (resourceLoader) {
		// Overriding loader
		newRes = resourceLoader(assetId, priority);
	} else {
		// Normal loading
		auto resLoader = ResourceLoader(*(parent.locator), assetId, type, priority, parent.api, parent);		
		newRes = loadResource(resLoader);
		if (newRes) {
			newRes->setMeta(resLoader.getMeta());
		} else if (resLoader.loaded) {
			throw Exception("Unable to construct resource from data: " + toString(type) + ":" + assetId, HalleyExceptions::Resources);
		}
	}

	if (!newRes) {
		if (allowFallback && !fallback.isEmpty()) {
			Logger::logError("Resource not found: \"" + toString(type) + ":" + assetId + "\"");
			return { loadAsset(fallback, priority, false).first, false };
		}
		
		throw Exception("Resource not found: \"" + toString(type) + ":" + assetId + "\"", HalleyExceptions::Resources);
	}

	newRes->setAssetId(assetId);
	return std::make_pair(newRes, true);
}

Vector<std::shared_ptr<Resource>> ResourceCollectionBase::getAllResources() const
{
	Vector<std::shared_ptr<Resource>> result;
	SharedLock lock(mutex);
	result.reserve(resources.size());
	for (auto& r: resources) {
		if (r.res) {
			result += r.res;
		}
	}
	return result;
}

#ifdef VIRTUAL_RESOURCE_GET
std::shared_ptr<Resource> ResourceCollectionBase::get(std::string_view assetId, ResourceLoadPriority priority, bool allowFallback)
{
	return doGet(assetId, priority, allowFallback);
}
#endif

std::shared_ptr<Resource> ResourceCollectionBase::doGet(std::string_view assetId, ResourceLoadPriority priority, bool allowFallback)
{
	using namespace std::chrono_literals;

	while (true) {
		{
			// First of all, look in cache and return if it's there
			// This is the most common case, and we use a shared lock to avoid stopping other threads
			SharedLock lock(mutex);
			if (const auto res = resourceMap.find(assetId); res != resourceMap.end()) {
				// Found resource, all good
				return resources[res->second].res;
			}
		}

		{
			// Resource is not cached
			// Another thread might already be loading it - check for that, and wait if so
			SharedLock lock(loadingMutex);
			if (resourcesLoading.contains(assetId)) {
				do {
					resourceLoaded.waitFor(lock, 100us);
				} while (resourcesLoading.contains(assetId));
				lock.unlock();

				// Should be loaded, try again
				SharedLock lock2(mutex);
				if (const auto res = resourceMap.find(assetId); res != resourceMap.end()) {
					// Found resource
					return resources[res->second].res;
				} else {
					// This means the previous one failed, we're giving up
					Logger::logError("Resource not found after waiting for it to be loaded elsewhere: \"" + toString(type) + ":" + assetId + "\"");
					return {};
				}
			}
		}

		// If we got here, nobody is trying to load this resource (yet)
		// We'll try to claim ownership of loading this resource

		{
			// Resource not found; claim loading it
			UniqueLock lock(loadingMutex);
			if (resourcesLoading.contains(assetId)) {
				// Someone else already loading it, between our last lock and now.
				// Wait until signaled then do the whole thing again
				resourceLoaded.waitFor(lock, 100us);
				continue;
			}
			// Claim loading, we're now responsible for this resource being loaded
			resourcesLoading.insert(assetId);
		}

		// Load resource from disk
		std::shared_ptr<Resource> newRes;
		bool loaded = false;
		try {
			std::tie(newRes, loaded) = loadAsset(assetId, priority, allowFallback);
		} catch (...) {
			UniqueLock lock(loadingMutex);
			resourcesLoading.erase(assetId);
			resourceLoaded.notifyAll();
			throw;
		}

		// Post-process
		if (loaded) {
			newRes->onLoaded(parent);
		}

		// Store in cache
		if (loaded) {
			UniqueLock lock2(mutex);
			newRes->setAssetIdx(curIdx++);
			allocateWrapper(assetId).res = newRes;
		}

		// Notify done loading
		{
			UniqueLock lock(loadingMutex);
			resourcesLoading.erase(assetId);
		}
		if (loaded) {
			resourceLoaded.notifyAll();
		}

		return newRes;
	}
}

ResourceCollectionBase::Wrapper& ResourceCollectionBase::allocateWrapper(const String& id)
{
	if (freeIdxs.empty()) {
		resourceMap.emplace(id, static_cast<uint32_t>(resources.size()));
		return resources.emplace_back();
	} else {
		const auto idx = freeIdxs.back();
		freeIdxs.pop_back();
		resourceMap.emplace(id, idx);
		return resources[idx];
	}
}

bool ResourceCollectionBase::exists(std::string_view assetId) const
{
	// Look in cache
	SharedLock lock(mutex);
	if (resourceMap.contains(assetId)) {
		return true;
	}
	lock.unlock();

	return parent.locator->exists(assetId, type);
}

void ResourceCollectionBase::setFallback(std::string_view assetId)
{
	fallback = assetId;
}

void ResourceCollectionBase::setResourceLoader(ResourceLoaderFunc loader)
{
	resourceLoader = std::move(loader);
}

void ResourceCollectionBase::setResourceEnumerator(ResourceEnumeratorFunc enumerator)
{
	resourceEnumerator = std::move(enumerator);
}
