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

ResourceCollectionBase::ResourceCollectionBase(Resources& parent, const String& path)
	: parent(parent)
	, path(path)
{
}

void ResourceCollectionBase::clear()
{
	resources.clear();
}

void ResourceCollectionBase::unload(const String& name)
{
	String fullName = resolveName(name);
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

void ResourceCollectionBase::flush(const String& name)
{
	auto res = resources.find(name);
	if (res != resources.end()) {
		auto& resWrap = res->second;
		resWrap.flush();
	}
}

String ResourceCollectionBase::resolveName(const String& name) const
{
	return path + "/" + name;
}

std::shared_ptr<Resource> ResourceCollectionBase::doGet(const String& rawName, ResourceLoadPriority priority)
{
	String name = resolveName(rawName);

	// Look in cache and return if it's there
	auto res = resources.find(name);
	if (res != resources.end()) {
		return res->second.res;
	}

	// Load metadata
	auto metaData = parent.locator->tryGetResource(name + ".meta", false);
	std::unique_ptr<Metadata> meta;
	if (metaData) {
		meta = Metadata::fromBinary(*static_cast<ResourceDataStatic*>(metaData.get()));
	} else {
		meta.reset(new Metadata());
	}

	// Load resource from disk
	auto resLoader = ResourceLoader(*(parent.locator), rawName, name, priority, parent.api, std::move(meta));
	auto newRes = loadResource(resLoader);
	if (!newRes) {
		if (resLoader.loaded) {
			throw Exception("Unable to construct resource from data: " + name);
		} else {
			throw Exception("Unable to load resource data: " + name);
		}
	}

	// Store in cache
	resources.emplace(name, Wrapper(newRes, 0));

	return newRes;
}

void ResourceCollectionBase::setResource(int curDepth, const String& name, std::shared_ptr<Resource> resource) {
	resources.emplace(name, Wrapper(resource, curDepth));
}
