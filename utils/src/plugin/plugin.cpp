#include "plugin.h"
#include <map>
#include <iostream>

class PluginStorage
{
public:
	std::map<Halley::PluginType, std::vector<std::unique_ptr<Halley::Plugin>>> plugins;
	
	static PluginStorage& getInstance()
	{
		static PluginStorage* ptr = nullptr;
		if (!ptr) {
			ptr = new PluginStorage();
		}
		return *ptr;
	}
};

void Halley::Plugin::registerPlugin(std::unique_ptr<Plugin> plugin)
{
	std::cout << "Hello world!" << std::endl;
	PluginStorage::getInstance().plugins[plugin->getType()].emplace_back(std::move(plugin));
}

std::vector<Halley::Plugin*> Halley::Plugin::getPlugins(PluginType type)
{
	std::vector<Halley::Plugin*> result;
	for (auto& p : PluginStorage::getInstance().plugins[type]) {
		result.push_back(&*p);
	}
	return result;
}
