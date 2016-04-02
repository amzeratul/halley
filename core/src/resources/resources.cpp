#include "resources.h"
#include "resource_locator.h"

using namespace Halley;

Resources::Resources() = default;
Resources::~Resources() = default;

void Resources::setBasePath(String path)
{
	basePath = path;
}

void Resources::clear()
{
	resources.clear();
}

void Resources::unload(String name)
{
	resources.erase(name);
}

void Resources::unloadAll(int minDepth)
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

void Resources::flush(String name)
{
	auto res = resources.find(name);
	if (res != resources.end()) {
		auto& resWrap = res->second;
		resWrap.flush();
	}
}

void Resources::flushAll(int minDepth)
{
	for (auto i = resources.begin(); i != resources.end(); i++) {
		auto& res = (*i).second;
		String name = (*i).first;
		auto curTime = getFileWriteTime(name);
		if (res.depth >= minDepth && res.lastWriteTime != curTime) {
			std::cout << "Flushing \"" << ConsoleColor(Console::DARK_GREY) << name << ConsoleColor() << "\"...\n";
			res.flush();
			res.lastWriteTime = curTime;
		}
	}
}

void Resources::setResource(String _name, std::shared_ptr<Resource> resource) {
	String name = resolveName(_name);
	resources.emplace(name, Wrapper(resource, curDepth, 0));
}

String Resources::getFullName(String name) const
{
	return resolveName(name);
}

String Resources::getBasePath() const 
{
	return basePath;
}

void Resources::setDepth(int depth) 
{
	curDepth = depth;
}

String Resources::resolveName(String name) const 
{
	return basePath + name;
}

std::shared_ptr<Resource> Resources::doGet(String _name, ResourceLoadPriority::Type priority, std::function<std::unique_ptr<Resource>(String, ResourceLoadPriority::Type)> loader)
{
	String name = resolveName(_name);

	// Look in cache
	auto res = resources.find(name);
	if (res != resources.end()) return res->second.res;

	// Not found, load it from disk
	auto newRes = loader(name, priority);
	if (!newRes) throw Exception("Unable to find resource: "+name);
	auto sharedResource = std::shared_ptr<Resource>(move(newRes));

	// Store in cache
	time_t time = getFileWriteTime(name);
	resources.emplace(name, Wrapper(sharedResource, curDepth, time));

	return sharedResource;
}

time_t Resources::getFileWriteTime(String name)
{
	return locator->getTimestamp(name);
}
