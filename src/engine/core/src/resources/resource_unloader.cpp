#include "halley/resources/resource_unloader.h"

using namespace Halley;

ResourceUnloader::ResourceUnloader(Resources& resources)
	: resources(resources)
{
	// TODO: move to game code
	constexpr auto mb = static_cast<size_t>(1024) * 1024;
	setBudget(AssetType::Texture, 1024 * mb);
	//setBudget(AssetType::AudioClip, 256 * mb);
}

void ResourceUnloader::setBudget(AssetType type, size_t maxSize)
{
	budgets[type] = maxSize;
}

void ResourceUnloader::update(Time t)
{
	for (auto& [type, budget]: budgets) {
		updateCollection(t, resources.ofType(type), budget);
	}
}

void ResourceUnloader::updateCollection(Time t, ResourceCollectionBase& collection, size_t sizeLimit)
{
	if (!collection.isAsync()) {
		return;
	}

	collection.forEachResource([&] (const std::shared_ptr<Resource>& resource) {
		auto& res = dynamic_cast<AsyncResource&>(*resource);

		res.startFrame(t);
		const auto usage = res.getUsageData();

		if (usage.loaded && usage.timeSinceInUse > 1.0 && usage.timeSinceInBackground >= 1.0) {
			Logger::logDev(String("Can probably unload resource [") + toString(collection.getAssetType()) + "] " + res.getAssetId(), true);
		}
	});
}
