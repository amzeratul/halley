#include "halley/tools/assets/asset_importer.h"
#include "halley/support/exception.h"
#include "importers/copy_file_importer.h"
#include "importers/font_importer.h"

using namespace Halley;


AssetImporter::AssetImporter()
{
	importers[AssetType::SIMPLE_COPY] = std::make_unique<CopyFileImporter>();
	importers[AssetType::FONT] = std::make_unique<FontImporter>();
}

IAssetImporter& AssetImporter::getImporter(Path path) const
{
	AssetType type = AssetType::SIMPLE_COPY;
	
	auto root = path.begin()->string();
	if (root == "font") {
		type = AssetType::FONT;
	}

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
