#include "resource_pack.h"
using namespace Halley;

PackResourceLocator::PackResourceLocator(SystemAPI& system, const Path& filePath, const String& encryptionKey, bool preLoad)
	: system(system)
{
}

std::unique_ptr<ResourceData> PackResourceLocator::getData(const String& asset, AssetType type, bool stream)
{
	return {};
}

const AssetDatabase& PackResourceLocator::getAssetDatabase() const
{
	return *assetDb;
}
