#include "halley/tools/assets/asset_importer.h"
#include "halley/support/exception.h"
#include "importers/copy_file_importer.h"
#include "importers/font_importer.h"
#include "importers/codegen_importer.h"
#include "importers/image_importer.h"
#include "importers/animation_importer.h"
#include "importers/material_importer.h"
#include "importers/config_importer.h"
#include "importers/audio_importer.h"
#include "importers/aseprite_importer.h"
#include "importers/spritesheet_importer.h"
#include "importers/bitmap_font_importer.h"
#include "halley/text/string_converter.h"

using namespace Halley;


AssetImporter::AssetImporter(std::vector<Path> assetsSrc)
{
	importers[AssetType::SimpleCopy] = std::make_unique<CopyFileImporter>();
	importers[AssetType::Font] = std::make_unique<FontImporter>();
	importers[AssetType::BitmapFont] = std::make_unique<BitmapFontImporter>();
	importers[AssetType::Image] = std::make_unique<ImageImporter>();
	importers[AssetType::Animation] = std::make_unique<AnimationImporter>();
	importers[AssetType::Material] = std::make_unique<MaterialImporter>();
	importers[AssetType::Config] = std::make_unique<ConfigImporter>();
	importers[AssetType::Codegen] = std::make_unique<CodegenImporter>();
	importers[AssetType::Audio] = std::make_unique<AudioImporter>();
	importers[AssetType::Aseprite] = std::make_unique<AsepriteImporter>();
	importers[AssetType::SpriteSheet] = std::make_unique<SpriteSheetImporter>();

	for (auto& i: importers) {
		i.second->setAssetsSrc(assetsSrc);
	}
}

IAssetImporter& AssetImporter::getImporter(Path path) const
{
	AssetType type = AssetType::SimpleCopy;
	
	auto root = path.getRoot();
	if (root == "font") {
		type = AssetType::Font;
	} else if (root == "bitmap_font") {
		type = AssetType::BitmapFont;
	} else if (root == "image") {
		type = AssetType::Image;
	} else if (root == "animation") {
		type = AssetType::Animation;
	} else if (root == "material") {
		type = AssetType::Material;
	} else if (root == "config") {
		type = AssetType::Config;
	} else if (root == "audio") {
		type = AssetType::Audio;
	} else if (root == "aseprite") {
		type = AssetType::Aseprite;
	} else if (root == "spritesheet") {
		type = AssetType::SpriteSheet;
	}

	return getImporter(type);
}

IAssetImporter& AssetImporter::getImporter(AssetType type) const
{
	auto i = importers.find(type);
	if (i != importers.end()) {
		return *i->second;
	} else {
		throw Exception("Unknown asset type: " + toString(int(type)));
	}
}
