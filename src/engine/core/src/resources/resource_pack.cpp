#include "resource_pack.h"
#include "api/system_api.h"
#include "resources/asset_pack.h"
using namespace Halley;

PackResourceLocator::PackResourceLocator(SystemAPI& system, const Path& filePath, const String& encryptionKey, bool preLoad)
{
	auto dataReader = system.getDataReader(filePath.string());
	if (!dataReader) {
		throw Exception("Unable to load resource pack \"" + filePath.string() + "\"");
	}
	assetPack = std::make_unique<AssetPack>(std::move(dataReader), encryptionKey, preLoad);
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
