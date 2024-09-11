#include "resource_pack.h"
#include <utility>
#include "halley/resources/asset_pack.h"
#include "halley/api/system_api.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

PackResourceLocator::PackResourceLocator(std::unique_ptr<ResourceDataReader> reader, Path path, std::optional<Encrypt::AESKey> key, bool preLoad, std::optional<int> priority)
	: path(std::move(path))
	, wasEncrypted(key.has_value())
	, preLoad(preLoad)
	, priority(priority)
{
	assetPack = std::make_unique<AssetPack>(std::move(reader), key, preLoad);
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
	if (wasEncrypted) {
		throw Exception("Attempting to hot reload a pack, but key has been lost.", HalleyExceptions::Resources);
	}
	assetPack = std::make_unique<AssetPack>(system->getDataReader(path.string()), std::nullopt, preLoad);
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
