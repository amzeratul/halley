#include "halley/data_structures/config_database.h"

#include "halley/concurrency/concurrent.h"
#include "halley/resources/resources.h"
#include "halley/file_formats/config_file.h"
#include "halley/game/game_platform.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

namespace {
#ifdef DEV_BUILD
	constexpr bool doAllowHotReload = isPCPlatform();
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
	const auto trace = StackDebugTrace("configName", configName);

	loadConfig(configFile->getRoot(), true);

	if (allowHotReload) {
		auto lock = UniqueLock(mutex);
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

namespace {
	const ConfigNode& processExtends(const String& type, const ConfigNode& nodes, ConfigNode& result)
	{
		if (nodes.getType() != ConfigNodeType::Sequence) {
			return nodes;
		}

		Vector<size_t> pendingLoad;
		HashSet<String> dependencies;
		{
			const auto& vs = nodes.asSequence();

			// Collect dependency info
			for (size_t i = 0; i < vs.size(); ++i) {
				const auto& v = vs[i];
				if (v.hasKey("$extends")) {
					pendingLoad += i;
					dependencies.insert(v["$extends"].asString());
				}
			}

			if (dependencies.empty()) {
				return nodes;
			}
		}

		result = ConfigNode(nodes);
		auto& vs = result.asSequence();

		// Create map and populate with root configs
		HashMap<String, size_t> idMap;
		HashSet<String> loadedIds;
		for (size_t i = 0; i < vs.size(); ++i) {
			const auto& id = vs[i]["id"].asString();
			if (dependencies.contains(id)) {
				idMap[id] = i;
				if (!vs[i].hasKey("$extends")) {
					loadedIds.insert(id);
				}
			}
		}

		// Keep trying to load pending
		while (!pendingLoad.empty()) {
			Vector<size_t> stillPending;
			for (auto& i: pendingLoad) {
				auto& v = vs[i];
				const auto& extends = v["$extends"].asString();
				if (loadedIds.contains(extends)) {
					loadedIds.insert(v["id"].asString());
					auto v2 = ConfigNode(vs[idMap.at(extends)]);
					for (auto& [key, value]: v.asMap()) {
						if (key != "$extends") {
							v2[key] = std::move(value);
						}
					}
					v = std::move(v2);
				} else {
					stillPending += i;
				}
			}
			if (pendingLoad == stillPending) {
				Logger::logError("Unable to process " + type + " config dependencies; is there a circular reference? First affected id: \"" + vs[stillPending[0]]["id"].asString("") + "\"");
				return nodes;
			}
			pendingLoad = std::move(stillPending);
		}

		return result;
	}
}

void ConfigDatabase::loadConfig(const ConfigNode& node, bool enforceUnique)
{
	if (node.getType() == ConfigNodeType::Map) {
		for (const auto& [k, vs]: node.asMap()) {
			if (onlyLoad && !std_ex::contains(*onlyLoad, k)) {
				continue;
			}

			for (auto& db: dbs) {
				if (db && db->getKey() == k) {
					try {
						ConfigNode temp;
						db->loadConfigs(processExtends(k, vs, temp), enforceUnique);
					} catch (const std::exception& e) {
						Logger::logException(e);
					}
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
