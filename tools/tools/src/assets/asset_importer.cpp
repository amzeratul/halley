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
#include "importers/shader_importer.h"
#include "halley/text/string_converter.h"
#include "halley/tools/project/project.h"
#include <boost/variant/detail/substitute.hpp>

using namespace Halley;

AssetImporter::AssetImporter(Project& project, std::vector<Path> assetsSrc)
{
	std::unique_ptr<IAssetImporter> defaultImporters[] = {
		std::make_unique<CopyFileImporter>(),
		std::make_unique<FontImporter>(),
		std::make_unique<BitmapFontImporter>(),
		std::make_unique<ImageImporter>(),
		std::make_unique<AnimationImporter>(),
		std::make_unique<MaterialImporter>(),
		std::make_unique<ConfigImporter>(),
		std::make_unique<CodegenImporter>(),
		std::make_unique<AudioImporter>(),
		std::make_unique<AsepriteImporter>(),
		std::make_unique<SpriteSheetImporter>(),
		std::make_unique<ShaderImporter>()
	};

	for (auto& i: defaultImporters) {
		auto type = i->getType();
		auto overrider = project.getAssetImporterOverride(type);
		auto& importer = overrider ? overrider : i;
		importer->setAssetsSrc(assetsSrc);
		importers[type] = std::move(importer);
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
	} else if (root == "shader") {
		type = AssetType::Shader;
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
