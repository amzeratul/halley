#include "halley/tools/packer/asset_packer.h"
#include "halley/core/resources/asset_database.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file/byte_serializer.h"
#include "halley/support/logger.h"
#include "halley/tools/packer/asset_pack_manifest.h"
using namespace Halley;

void AssetPacker::pack(const AssetPackManifest& manifest, const Path& assetsDir, const Path& dst)
{
	// Retrieve assets database
	auto assetDbData = FileSystem::readFile(assetsDir / "assets.db");
	if (assetDbData.empty()) {
		throw Exception("Unable to find assets.db at \"" + assetsDir.string() + "\"");
	}
	AssetDatabase db = Deserializer::fromBytes<AssetDatabase>(assetDbData);
	
	for (auto& assetName: db.getAssets()) {
		auto pack = manifest.getPack(assetName);
		if (pack) {
			Logger::logInfo(assetName + " -> " + pack.get().get().getName());
		} else {
			Logger::logWarning(assetName + " -> [none]");
		}		
	}
}
