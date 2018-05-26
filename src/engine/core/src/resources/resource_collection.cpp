#include "resources/resource_collection.h"
#include "resources/resource_locator.h"
#include "resources/resources.h"
#include <halley/resources/resource.h>
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
		resWrap.res->reload(std::move(*loadAsset(assetId, ResourceLoadPriority::High)));
	}
}

std::vector<String> ResourceCollectionBase::enumerate() const
{
	return parent.locator->enumerate(type);
}

std::shared_ptr<Resource> ResourceCollectionBase::loadAsset(const String& assetId, ResourceLoadPriority priority) {
	std::shared_ptr<Resource> newRes;

	if (resourceLoader) {
		// Overriding loader
		newRes = resourceLoader(assetId, priority);
	} else {
		// Normal loading
		auto resLoader = ResourceLoader(*(parent.locator), assetId, type, priority, parent.api);
		newRes = loadResource(resLoader);
		if (!newRes && resLoader.loaded) {
			throw Exception("Unable to construct resource from data: " + assetId);
		}
	}

	if (!newRes) {
		throw Exception("Unable to load resource data: " + assetId);
	}
	return newRes;
}

std::shared_ptr<Resource> ResourceCollectionBase::doGet(const String& assetId, ResourceLoadPriority priority)
{
	// Look in cache and return if it's there
	auto res = resources.find(assetId);
	if (res != resources.end()) {
		return res->second.res;
	}
	
	// Load resource from disk
	std::shared_ptr<Resource> newRes = loadAsset(assetId, priority);

	// Store in cache
	newRes->setAssetId(assetId);
	resources.emplace(assetId, Wrapper(newRes, 0));

	return newRes;
}

bool ResourceCollectionBase::exists(const String& assetId)
{
	// Look in cache
	auto res = resources.find(assetId);
	if (res != resources.end()) {
		return true;
	}

	return parent.locator->exists(assetId);
}

void ResourceCollectionBase::setResource(int curDepth, const String& name, std::shared_ptr<Resource> resource) {
	resources.emplace(name, Wrapper(resource, curDepth));
}

void ResourceCollectionBase::setResourceLoader(ResourceLoaderFunc loader)
{
	resourceLoader = loader;
}
