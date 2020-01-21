#include "resource_pack.h"
#include <utility>
#include "resources/asset_pack.h"
#include "api/system_api.h"
using namespace Halley;

PackResourceLocator::PackResourceLocator(std::unique_ptr<ResourceDataReader> reader, Path path, String key, bool preLoad, Maybe<int> priority)
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

void PackResourceLocator::purge(SystemAPI& sys)
{
	assetPack.reset();
	system = &sys;
}

void PackResourceLocator::loadAfterPurge()
{
	assetPack = std::make_unique<AssetPack>(system->getDataReader(path.string()), encryptionKey, preLoad);
}

int PackResourceLocator::getPriority() const
{
	return priority ? priority.value() : IResourceLocatorProvider::getPriority();
}
