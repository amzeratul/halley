#include "halley/tools/assets/asset_importer.h"
#include "halley/support/exception.h"

using namespace Halley;


AssetImporter::AssetImporter()
{
	// TODO
}

IAssetImporter& AssetImporter::getImporter(Path path) const
{
	AssetType type = AssetType::UNDEFINED;
	// TODO

	return getImporter(type);
}

IAssetImporter& AssetImporter::getImporter(AssetType type) const
{
	auto i = importers.find(type);
	if (i != importers.end()) {
		return *i->second;
	} else {
		throw Exception("Unknown asset type: " + String::integerToString(int(type)));
	}
}
