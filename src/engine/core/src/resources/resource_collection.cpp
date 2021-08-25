#include "resources/resource_collection.h"
#include "resources/resource_locator.h"
#include "resources/resources.h"
#include <halley/resources/resource.h>
#include <utility>

#include "graphics/sprite/sprite.h"
#include "halley/support/logger.h"

using namespace Halley;


ResourceCollectionBase::ResourceCollectionBase(Resources& parent, AssetType type)
	: parent(parent)
	, type(type)
{
}

void ResourceCollectionBase::clear()
{
	resources.clear();
}

void ResourceCollectionBase::unload(const String& assetId)
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

void ResourceCollectionBase::reload(const String& assetId)
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
			Logger::logError("Error while reloading " + assetId + ": " + e.what());
		} catch (...) {
			Logger::logError("Unknown error while reloading " + assetId);
		}
	}
}

void ResourceCollectionBase::purge(const String& assetId)
{
	if (!resourceLoader) {
		parent.locator->purge(assetId, type);
	}
}

std::shared_ptr<Resource> ResourceCollectionBase::getUntyped(const String& name, ResourceLoadPriority priority)
{
	return doGet(name, priority, true);
}

std::vector<String> ResourceCollectionBase::enumerate() const
{
	if (resourceEnumerator) {
		return resourceEnumerator();
	} else {
		return parent.locator->enumerate(type);
	}
}

std::pair<std::shared_ptr<Resource>, bool> ResourceCollectionBase::loadAsset(const String& assetId, ResourceLoadPriority priority, bool allowFallback) {
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
	
	return std::make_pair(newRes, true);
}

std::shared_ptr<Resource> ResourceCollectionBase::doGet(const String& assetId, ResourceLoadPriority priority, bool allowFallback)
{
	// Look in cache and return if it's there
	const auto res = resources.find(assetId);
	if (res != resources.end()) {
		return res->second.res;
	}
	
	// Load resource from disk
	const auto [newRes, loaded] = loadAsset(assetId, priority, allowFallback);

	// Store in cache
	if (loaded) {
		newRes->setAssetId(assetId);
		resources.emplace(assetId, Wrapper(newRes, 0));
		newRes->onLoaded(parent);
	}

	return newRes;
}

bool ResourceCollectionBase::exists(const String& assetId) const
{
	// Look in cache
	const auto res = resources.find(assetId);
	if (res != resources.end()) {
		return true;
	}

	return parent.locator->exists(assetId, type);
}

void ResourceCollectionBase::setFallback(const String& assetId)
{
	fallback = assetId;
}

void ResourceCollectionBase::setResource(int curDepth, const String& name, std::shared_ptr<Resource> resource) {
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
