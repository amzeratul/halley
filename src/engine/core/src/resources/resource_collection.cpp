#include "halley/resources/resource_collection.h"
#include "halley/resources/resource_locator.h"
#include "halley/resources/resources.h"
#include <halley/resources/resource.h>
#include <utility>

#include "halley/graphics/sprite/sprite.h"
#include "halley/support/logger.h"
#include "halley/utils/scoped_guard.h"

using namespace Halley;



#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace {
	HMODULE getCurrentModuleHandle()
	{
		HMODULE hMod = nullptr;
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&getCurrentModuleHandle), &hMod);
		return hMod;
	}

	constexpr bool checkForLoadingFromDLL = false;

	bool isRunningFromDLL()
	{
		if (!checkForLoadingFromDLL) {
			return false;
		}

#ifdef DEV_BUILD
		char name[1024];
		GetModuleFileNameA(getCurrentModuleHandle(), name, sizeof(name));
		auto str = std::string_view(name);
		return str.length() > 4 && str.substr(str.length() - 4, 4) == ".dll";
#else
		return false;
#endif
	}
}
#else
namespace {
	bool isRunningFromDLL()
	{
		return false;
	}
}
#endif

ResourceCollectionBase::ResourceCollectionBase(Resources& parent, AssetType type)
	: parent(parent)
	, type(type)
{
	if (isRunningFromDLL()) {
		throw Exception("ResourceCollectionBase for " + toString(type) + " is being created within DLL code", HalleyExceptions::Resources);
	}
}

void ResourceCollectionBase::clear()
{
	resources.clear();
}

void ResourceCollectionBase::unload(std::string_view assetId)
{
	resources.erase(assetId);
}

void ResourceCollectionBase::unloadAll(int minDepth)
{
	for (auto iter = resources.begin(); iter != resources.end(); ) {
		auto next = iter;
		++next;

		auto& res = (*iter).second;
		if (res.depth >= minDepth) {
			resources.erase(iter);
		}

		iter = next;
	}
}

void ResourceCollectionBase::reload(std::string_view assetId)
{
	auto res = resources.find(assetId);
	if (res != resources.end()) {
		auto& resWrap = res->second;
		auto& oldAsset = *resWrap.res;
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
		usage += r.second.res->getMemoryUsage();
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
			const auto desiredState = isAsync() ?
				dynamic_cast<AsyncResource&>(*r.second.res).getDesiredLoadState() :
				ResourceDesiredLoadState::Undefined;
			usages[desiredState].emplace_back(Info{ r.first, r.second.res->getMemoryUsage(), static_cast<int>(r.second.res.use_count()) });
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

void ResourceCollectionBase::age(float time)
{
	SharedLock lock(mutex);

	for (auto& r: resources) {
		const auto& resourcePtr = r.second.res;
		// This code is dodgy
		// It's designed for Texture, but that's held by a shared_ptr in SpriteSheet
		if (resourcePtr.use_count() <= 2) {
			resourcePtr->increaseAge(time);
		} else {
			resourcePtr->resetAge();
		}
	}
}

ResourceMemoryUsage ResourceCollectionBase::clearOldResources(float maxAge)
{
	Vector<decltype(resources)::iterator> toDelete;
	ResourceMemoryUsage usage;

	{
		SharedLock lock(mutex);

		for (auto iter = resources.begin(); iter != resources.end(); ) {
			auto next = iter;
			++next;

			auto& resourcePtr = iter->second.res;
			if (resourcePtr.use_count() <= 2 && resourcePtr->getAge() > maxAge) {
				usage += resourcePtr->getMemoryUsage();
				resourcePtr->setUnloaded();
				toDelete.push_back(iter);
			}

			iter = next;
		}

		for (auto& a: toDelete) {
			resources.erase(a);
		}
	}

	// Delete out of the lock to avoid stalling resources for too long
	toDelete.clear();

	return usage;
}

void ResourceCollectionBase::notifyResourcesUnloaded()
{
	SharedLock lock(mutex);

	for (auto& r: resources) {
		r.second.res->onOtherResourcesUnloaded();
	}
}

ResourceMemoryUsage ResourceCollectionBase::getMemoryUsageAndAge(float time)
{
	ResourceMemoryUsage usage;
	SharedLock lock(mutex);

	for (auto& r: resources) {
		auto& resourcePtr = r.second.res;
		if (resourcePtr.use_count() <= 2) {
			resourcePtr->increaseAge(time);
		} else {
			resourcePtr->resetAge();
		}
		usage += resourcePtr->getMemoryUsage();
	}

	return usage;
}

std::pair<std::shared_ptr<Resource>, bool> ResourceCollectionBase::loadAsset(std::string_view assetId, ResourceLoadPriority priority, bool allowFallback)
{
	if (isRunningFromDLL()) {
		throw Exception("Asset " + toString(type) + ":" + String(assetId) + " is being loaded within DLL code", HalleyExceptions::Resources);
	}

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
			if (const auto res = resources.find(assetId); res != resources.end()) {
				// Found resource, all good
				return res->second.res;
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
				if (const auto res = resources.find(assetId); res != resources.end()) {
					// Found resource
					return res->second.res;
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
			resources.emplace(assetId, Wrapper(newRes, 0));
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

bool ResourceCollectionBase::exists(std::string_view assetId) const
{
	// Look in cache
	SharedLock lock(mutex);
	const auto res = resources.find(assetId);
	if (res != resources.end()) {
		return true;
	}
	lock.unlock();

	return parent.locator->exists(assetId, type);
}

void ResourceCollectionBase::setFallback(std::string_view assetId)
{
	fallback = assetId;
}

void ResourceCollectionBase::setResource(int curDepth, std::string_view name, std::shared_ptr<Resource> resource) {
	resources.emplace(name, Wrapper(std::move(resource), curDepth));
}

void ResourceCollectionBase::setResourceLoader(ResourceLoaderFunc loader)
{
	resourceLoader = std::move(loader);
}

void ResourceCollectionBase::setResourceEnumerator(ResourceEnumeratorFunc enumerator)
{
	resourceEnumerator = std::move(enumerator);
}
