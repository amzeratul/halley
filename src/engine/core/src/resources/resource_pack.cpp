#include "resource_pack.h"
#include "resources/asset_pack.h"
using namespace Halley;

PackResourceLocator::PackResourceLocator(std::unique_ptr<ResourceDataReader> reader, const String& encryptionKey, bool preLoad)
{
	assetPack = std::make_unique<AssetPack>(std::move(reader), encryptionKey, preLoad);
}

PackResourceLocator::~PackResourceLocator()
{
}

std::unique_ptr<ResourceData> PackResourceLocator::getData(const String& asset, AssetType type, bool stream)
{
	return assetPack->getData(asset, type, stream);
}

const AssetDatabase& PackResourceLocator::getAssetDatabase() const
{
	return assetPack->getAssetDatabase();
}
