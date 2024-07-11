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

	bool isRunningFromDLL()
	{
		char name[1024];
		GetModuleFileNameA(getCurrentModuleHandle(), name, sizeof(name));
		auto str = std::string_view(name);
		return str.length() > 4 && str.substr(str.length() - 4, 4) == ".dll";
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
	//assert(!isRunningFromDLL());
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
		try {
			const auto [newAsset, loaded] = loadAsset(assetId, ResourceLoadPriority::High, false);
			newAsset->setAssetId(assetId);
			newAsset->onLoaded(parent);
			resWrap.res->reloadResource(std::move(*newAsset));
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
	return doGet(name, priority, true);
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
	std::shared_lock lock(mutex);

	for (auto& r: resources) {
		usage += r.second.res->getMemoryUsage();
	}

	return usage;
}

void ResourceCollectionBase::age(float time)
{
	std::shared_lock lock(mutex);

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
		std::shared_lock lock(mutex);

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
	std::shared_lock lock(mutex);

	for (auto& r: resources) {
		r.second.res->onOtherResourcesUnloaded();
	}
}

ResourceMemoryUsage ResourceCollectionBase::getMemoryUsageAndAge(float time)
{
	ResourceMemoryUsage usage;
	std::shared_lock lock(mutex);

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
	//assert(!isRunningFromDLL());

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

std::shared_ptr<Resource> ResourceCollectionBase::doGet(std::string_view assetId, ResourceLoadPriority priority, bool allowFallback)
{
	using namespace std::chrono_literals;

	for (int i = 0; true; ++i) {
		{
			// Look in cache and return if it's there
			std::shared_lock lock(mutex);
			const auto res = resources.find(assetId);
			if (res != resources.end()) {
				// Found resource, all good
				return res->second.res;
			}
		}

		{
			// Resource not found; claim loading it
			std::unique_lock lock(mutex);
			if (resourcesLoading.contains(assetId)) {
				// Someone else already loading it, wait until signaled then do the whole thing again
				resourceLoaded.wait_for(lock, 20us);
				continue;
			}
			resourcesLoading.insert(assetId);
		}

		// Load resource from disk
		std::shared_ptr<Resource> newRes;
		bool loaded = false;
		try {
			std::tie(newRes, loaded) = loadAsset(assetId, priority, allowFallback);
		} catch (...) {
			std::unique_lock lock(mutex);
			resourcesLoading.erase(assetId);
			resourceLoaded.notify_all();
			throw;
		}

		// Store in cache
		{
			std::unique_lock lock(mutex);
			resourcesLoading.erase(assetId);
			if (loaded) {
				resources.emplace(assetId, Wrapper(newRes, 0));
				resourceLoaded.notify_all();
			}
		}

		if (loaded) {
			newRes->onLoaded(parent);
		}
		return newRes;
	}
}

bool ResourceCollectionBase::exists(std::string_view assetId) const
{
	// Look in cache
	const auto res = resources.find(assetId);
	if (res != resources.end()) {
		return true;
	}

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
