#include "halley/resources/resources.h"
#include "halley/resources/resource_locator.h"
#include "halley/api/halley_api.h"
#include "halley/support/logger.h"

using namespace Halley;

Resources::Resources(std::unique_ptr<ResourceLocator> locator, const HalleyAPI& api, ResourceOptions options)
	: locator(std::move(locator))
	, api(&api)
	, options(options)
{
}

Resources::~Resources()
{
	resources.clear();
}

void Resources::reloadAssets(const Vector<String>& ids, const Vector<String>& packIds)
{
	// Early out
	if (ids.empty()) {
		return;
	}

	// Build this map first, so it gets sorted by AssetType
	// The order in which asset types are reloaded is important, since they have dependencies
	std::map<AssetType, Vector<String>> byType;

	for (const auto& id: ids) {
		const auto splitPos = id.find(':');
		const auto type = fromString<AssetType>(id.left(splitPos));
		String name = id.mid(splitPos + 1);
		byType[type].emplace_back(std::move(name));
	}

	reloadAssets(byType);

	locator->purgePacks(ids, packIds);
}

void Resources::reloadAssets(const std::map<AssetType, Vector<String>>& byType)
{
	// Pause audio if needed
	bool needsAudioPause = false;
	for (auto& curType: byType) {
		if (curType.first == AssetType::AudioClip || curType.first == AssetType::AudioObject) {
			needsAudioPause = true;
			break;
		}
	}
	if (needsAudioPause) {
		Logger::logDev("Pausing audio playback...");
		api->audio->pausePlayback();
	}

	// Purge assets first, to force re-loading of any affected packs
	for (auto& curType: byType) {
		auto& res = ofType(curType.first);
		for (auto& asset: curType.second) {
			res.purge(asset);
		}
	}

	// Reload assets
	for (auto& curType: byType) {
		auto& res = ofType(curType.first);
		for (auto& asset: curType.second) {
			//Logger::logInfo("Reloading " + curType.first + ": " + asset);
			res.reload(asset);
		}
	}

	// Resume audio
	if (needsAudioPause) {
		Logger::logDev("Resuming audio playback...");
		api->audio->resumePlayback();
	}
}

void Resources::generateMemoryReport()
{
	Vector<std::pair<AssetType, ResourceMemoryUsage>> usage;
	ResourceMemoryUsage total;

	for (auto& res: resources) {
		if (res) {
			usage.emplace_back(res->getAssetType(), res->getMemoryUsage());
			total += usage.back().second;
		}
	}

	std::sort(usage.begin(), usage.end(), [](const auto& a, const auto& b) { return a.second.getTotal() > b.second.getTotal(); });

	Logger::logInfo("Resource memory usage: " + total.toString());

	for (const auto& [assetType, memoryUsage] : usage) {
		if (memoryUsage.ramUsage > 0 || memoryUsage.vramUsage > 0) {
			Logger::logInfo(String("\t") + toString(assetType) + ": " + memoryUsage.toString());
		}
	}

	locator->generateMemoryReport();
}

void Resources::generateDetailedMemoryReport(AssetType type, std::optional<int> limit)
{
	for (auto& res: resources) {
		if (res && res->getAssetType() == type) {
			res->generateDetailedMemoryReport(limit);
			return;
		}
	}
}
