#include <iostream>
#include <halley/support/console.h>
#include "resources/resource_collection.h"
#include "resources/resource_locator.h"
#include "resources/resources.h"
#include <halley/resources/metadata.h>
#include <halley/resources/resource.h>

using namespace Halley;

void ResourceCollectionBase::Wrapper::flush()
{
	// TODO
}

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

void ResourceCollectionBase::flush(const String& assetId)
{
	auto res = resources.find(assetId);
	if (res != resources.end()) {
		auto& resWrap = res->second;
		resWrap.flush();
	}
}

std::vector<String> ResourceCollectionBase::enumerate() const
{
	return parent.locator->enumerate(type);
}

std::shared_ptr<Resource> ResourceCollectionBase::doGet(const String& assetId, ResourceLoadPriority priority)
{
	// Look in cache and return if it's there
	auto res = resources.find(assetId);
	if (res != resources.end()) {
		return res->second.res;
	}
	
	// Load resource from disk
	auto resLoader = ResourceLoader(*(parent.locator), assetId, type, priority, parent.api);
	auto newRes = loadResource(resLoader);
	if (!newRes) {
		if (resLoader.loaded) {
			throw Exception("Unable to construct resource from data: " + assetId);
		} else {
			throw Exception("Unable to load resource data: " + assetId);
		}
	}

	// Store in cache
	newRes->setAssetId(assetId);
	resources.emplace(assetId, Wrapper(newRes, 0));

	return newRes;
}

void ResourceCollectionBase::setResource(int curDepth, const String& name, std::shared_ptr<Resource> resource) {
	resources.emplace(name, Wrapper(resource, curDepth));
}
