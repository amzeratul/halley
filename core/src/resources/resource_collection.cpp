#include "resource_collection.h"
#include "resource_locator.h"
#include "resources.h"

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
	for (auto i = resources.begin(); i != resources.end(); i++) {
		auto& res = (*i).second;
		String name = (*i).first;
		auto curTime = parent.getFileWriteTime(name);
		if (res.depth >= minDepth && res.lastWriteTime != curTime) {
			std::cout << "Flushing \"" << ConsoleColor(Console::DARK_GREY) << name << ConsoleColor() << "\"...\n";
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

	// Look in cache
	auto res = resources.find(name);
	if (res != resources.end()) return res->second.res;

	// Check if metafile exists
	auto metaData = parent.locator->tryGetResource(name + ".meta.yaml", false);
	std::unique_ptr<Metadata> meta;
	if (metaData) {
		meta.reset(new Metadata(*static_cast<ResourceDataStatic*>(metaData.get())));
	} else {
		meta.reset(new Metadata());
	}

	// Not found, load it from disk
	auto resLoader = ResourceLoader(*(parent.locator), rawName, name, priority, parent.api, std::move(meta));
	auto newRes = loadResource(resLoader);
	if (!newRes) {
		if (resLoader.loaded) {
			throw Exception("Unable to construct resource from data: " + name);
		}
		else {
			throw Exception("Unable to load resource data: " + name);
		}
	}
	auto sharedResource = std::shared_ptr<Resource>(std::move(newRes));

	// Store in cache
	time_t time = parent.getFileWriteTime(name);
	resources.emplace(name, Wrapper(sharedResource, parent.curDepth, time));

	return sharedResource;
}

void ResourceCollectionBase::setResource(int curDepth, String name, std::shared_ptr<Resource> resource) {
	resources.emplace(name, Wrapper(resource, curDepth, 0));
}
