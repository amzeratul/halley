#include "halley/data_structures/config_database.h"

#include "halley/concurrency/concurrent.h"
#include "halley/resources/resources.h"
#include "halley/file_formats/config_file.h"
#include "halley/game/game_platform.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

namespace {
#ifdef DEV_BUILD
	constexpr bool doAllowHotReload = getPlatform() != GamePlatform::Switch;
#else
	constexpr bool doAllowHotReload = false;
#endif
}

ConfigDatabase::ConfigDatabase(std::optional<Vector<String>> onlyLoad)
	: allowHotReload(doAllowHotReload)
	, onlyLoad(std::move(onlyLoad))
{
}

void ConfigDatabase::loadConfigs(Resources& resources, const std::function<bool(const String&)>& filter)
{
	bool threadedLoad = true;

	Vector<Future<void>> pending;

	for (auto configName: resources.enumerate<ConfigFile>()) {
		if (filter(configName)) {
			if (threadedLoad) {
				pending += Concurrent::execute([this, &resources, configName = std::move(configName)]
				{
					loadFile(resources, configName);
				});
			} else {
				loadFile(resources, configName);
			}
		}
	}

	if (threadedLoad) {
		Concurrent::whenAll(pending.begin(), pending.end()).wait();
	}
}

void ConfigDatabase::loadFile(Resources& resources, const String& configName)
{
	auto configFile = resources.get<ConfigFile>(configName);

	loadConfig(configFile->getRoot(), true);

	if (allowHotReload) {
		auto lock = std::unique_lock(mutex);
		observers[configFile->getAssetId()] = ConfigObserver(*configFile);
	} else {
		resources.unload<ConfigFile>(configName);
	}
}

int ConfigDatabase::getVersion() const
{
	return version;
}

void ConfigDatabase::generateMemoryReport()
{
	size_t totalSize = 0;
	std::map<String, size_t> results;
	for (const auto& db: dbs) {
		const auto size = db->getMemoryUsage();
		totalSize += size;
		results[db->getKey()] = size;
	}

	Logger::logInfo("ConfigDatabase memory usage: " + String::prettySize(totalSize));
	for (const auto& [k, v]: results) {
		Logger::logInfo("\t" + k + ": " + String::prettySize(v));
	}
}

void ConfigDatabase::loadConfig(const ConfigNode& node, bool enforceUnique)
{
	if (node.getType() == ConfigNodeType::Map) {
		for (const auto& [k, v]: node.asMap()) {
			if (onlyLoad && !std_ex::contains(*onlyLoad, k)) {
				continue;
			}

			for (auto& db: dbs) {
				if (db && db->getKey() == k) {
					db->loadConfigs(v, enforceUnique);
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
			loadConfig(o.getRoot(), false);
		}
	}

	if (changed) {
		++version;
	}
}

size_t ConfigDatabase::nextIdx = 0;
