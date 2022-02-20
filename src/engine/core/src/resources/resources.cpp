#include "resources/resources.h"
#include "resources/resource_locator.h"
#include "api/halley_api.h"
#include "halley/support/logger.h"

using namespace Halley;

Resources::Resources(std::unique_ptr<ResourceLocator> locator, const HalleyAPI& api, ResourceOptions options)
	: locator(std::move(locator))
	, api(&api)
	, options(options)
{
}

void Resources::reloadAssets(const Vector<String>& ids)
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

	locator->purgeAll();
}

void Resources::reloadAssets(const std::map<AssetType, Vector<String>>& byType)
{
	// Purge assets first, to force re-loading of any affected packs
	for (auto& curType: byType) {
		if (curType.first == AssetType::AudioClip) {
			api->audio->pausePlayback();
		}

		auto& resources = ofType(curType.first);
		for (auto& asset: curType.second) {
			resources.purge(asset);
		}
	}

	// Reload assets
	for (auto& curType: byType) {
		auto& resources = ofType(curType.first);
		for (auto& asset: curType.second) {
			//Logger::logInfo("Reloading " + curType.first + ": " + asset);
			resources.reload(asset);
		}

		if (curType.first == AssetType::AudioClip) {
			api->audio->resumePlayback();
		}
	}
}

Resources::~Resources() = default;
