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

ResourceCollectionBase::ResourceCollectionBase(Resources& parent, String path)
	: parent(parent)
	, path(path)
{
}

void ResourceCollectionBase::clear()
{
	resources.clear();
}

void ResourceCollectionBase::unload(String name)
{
	resources.erase(name);
}

void ResourceCollectionBase::unloadAll(int minDepth)
{
	for (auto iter = resources.begin(); iter != resources.end(); ) {
		auto next = iter;
		++next;

		auto& res = (*iter).second;
		if (res.depth >= minDepth) {
			//std::cout << "Unloaded " << (*iter).first << "\n";
			resources.erase(iter);
		}

		iter = next;
	}
}

void ResourceCollectionBase::flush(String name)
{
	auto res = resources.find(name);
	if (res != resources.end()) {
		auto& resWrap = res->second;
		resWrap.flush();
	}
}

void ResourceCollectionBase::flushAll(int minDepth)
{
	for (auto& iter: resources) {
		String name = iter.first;
		auto& res = iter.second;
		auto curTime = parent.getFileWriteTime(name);
		if (res.depth >= minDepth && res.lastWriteTime != curTime) {
			std::cout << "Flushing \"" << ConsoleColour(Console::DARK_GREY) << name << ConsoleColour() << "\"...\n";
			res.flush();
			res.lastWriteTime = curTime;
		}
	}
}

String ResourceCollectionBase::resolveName(String name) const
{
	return parent.basePath + "/" + path + "/" + name;
}

std::shared_ptr<Resource> ResourceCollectionBase::doGet(String rawName, ResourceLoadPriority priority)
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
		meta.reset(new Metadata(*static_cast<ResourceDataStatic*>(metaData.get())));
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
	time_t time = parent.getFileWriteTime(name);
	resources.emplace(name, Wrapper(newRes, parent.curDepth, time));

	return newRes;
}

void ResourceCollectionBase::setResource(int curDepth, String name, std::shared_ptr<Resource> resource) {
	resources.emplace(name, Wrapper(resource, curDepth, 0));
}
