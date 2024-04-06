#include "resource_pack.h"
#include <utility>
#include "halley/resources/asset_pack.h"
#include "halley/api/system_api.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

PackResourceLocator::PackResourceLocator(std::unique_ptr<ResourceDataReader> reader, Path path, String key, bool preLoad, std::optional<int> priority)
	: path(std::move(path))
	, encryptionKey(std::move(key))
	, preLoad(preLoad)
	, priority(priority)
{
	assetPack = std::make_unique<AssetPack>(std::move(reader), encryptionKey, preLoad);
}

PackResourceLocator::~PackResourceLocator()
{
}

std::unique_ptr<ResourceData> PackResourceLocator::getData(const String& asset, AssetType type, bool stream)
{
	if (!assetPack) {
		loadAfterPurge();
	}
	return assetPack->getData(asset, type, stream);
}

const AssetDatabase& PackResourceLocator::getAssetDatabase()
{
	if (!assetPack) {
		loadAfterPurge();
	}
	return assetPack->getAssetDatabase();
}

void PackResourceLocator::purgeAll(SystemAPI& sys)
{
	assetPack.reset();
	system = &sys;
}

bool PackResourceLocator::purgeIfAffected(SystemAPI& system, gsl::span<const String> assetIds, gsl::span<const String> packIds)
{
	const auto packId = path.getFilename().replaceExtension("").string();
	if (std_ex::contains(packIds, packId)) {
		Logger::logDev("Purging pack " + packId);
		assetPack.reset();
		this->system = &system;
		return true;
	}
	return false;
}

void PackResourceLocator::loadAfterPurge()
{
	assetPack = std::make_unique<AssetPack>(system->getDataReader(path.string()), encryptionKey, preLoad);
}

int PackResourceLocator::getPriority() const
{
	return priority ? priority.value() : IResourceLocatorProvider::getPriority();
}

size_t PackResourceLocator::getMemoryUsage() const
{
	return sizeof(*this) + (assetPack ? assetPack->getMemoryUsage() : 0);
}

String PackResourceLocator::getName() const
{
	return path.getFilenameStr();
}
