#include "halley/data_structures/config_database.h"

#include "halley/resources/resources.h"
#include "halley/file_formats/config_file.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

ConfigDatabase::ConfigDatabase(std::optional<Vector<String>> onlyLoad)
	: onlyLoad(std::move(onlyLoad))
{
}

void ConfigDatabase::load(Resources& resources, const String& prefix)
{
	for (const auto& configName: resources.enumerate<ConfigFile>()) {
		if (configName.startsWith(prefix)) {
			loadFile(*resources.get<ConfigFile>(configName));
		}
	}
}

void ConfigDatabase::loadFile(const ConfigFile& configFile)
{
#ifdef DEV_BUILD
	observers[configFile.getAssetId()] = ConfigObserver(configFile);
#endif
	loadConfig(configFile.getRoot());
}

int ConfigDatabase::getVersion() const
{
	return version;
}

void ConfigDatabase::loadConfig(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		for (const auto& [k, v]: node.asMap()) {
			if (onlyLoad && !std_ex::contains(*onlyLoad, k)) {
				continue;
			}

			for (auto& db: dbs) {
				if (db && db->getKey() == k) {
					db->loadConfigs(v);
					break;
				}
			}
		}
	}
}

void ConfigDatabase::update()
{
	bool changed = false;
	for (auto& [k, o]: observers) {
		if (o.needsUpdate()) {
			changed = true;
			o.update();
			loadConfig(o.getRoot());
		}
	}

	if (changed) {
		++version;
	}
}

size_t ConfigDatabase::nextIdx = 0;
