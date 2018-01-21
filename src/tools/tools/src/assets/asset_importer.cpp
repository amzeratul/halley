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
#include "importers/texture_importer.h"

using namespace Halley;

AssetImporter::AssetImporter(Project& project, const std::vector<Path>& assetsSrc)
	: assetsSrc(assetsSrc)
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
		std::make_unique<ShaderImporter>(),
		std::make_unique<TextureImporter>(),
		std::make_unique<IAssetImporter>()
	};

	for (auto& i: defaultImporters) {
		auto type = i->getType();
		auto overrider = project.getAssetImporterOverride(type);
		auto& importer = overrider ? overrider : i;
		importers[type] = std::move(importer);
	}
}

IAssetImporter& AssetImporter::getImporter(Path path) const
{
	ImportAssetType type = ImportAssetType::SimpleCopy;
	
	auto root = path.getRoot();
	if (root == "font") {
		type = ImportAssetType::Font;
	} else if (root == "bitmap_font") {
		type = ImportAssetType::BitmapFont;
	} else if (root == "image") {
		auto file = path.getFilename().toString();
		if (file.endsWith(".ase") || file.endsWith(".ase.meta")) {
			type = ImportAssetType::Aseprite;
		} else {
			type = ImportAssetType::Image;
		}
	} else if (root == "animation") {
		type = ImportAssetType::Animation;
	} else if (root == "material") {
		type = ImportAssetType::Material;
	} else if (root == "config") {
		type = ImportAssetType::Config;
	} else if (root == "audio") {
		type = ImportAssetType::Audio;
	} else if (root == "spritesheet") {
		type = ImportAssetType::SpriteSheet;
	} else if (root == "shader") {
		type = ImportAssetType::Skip;
	} else if (root == "texture") {
		type = ImportAssetType::Texture;
	}

	return getImporter(type);
}

IAssetImporter& AssetImporter::getImporter(ImportAssetType type) const
{
	auto i = importers.find(type);
	if (i != importers.end()) {
		return *i->second;
	} else {
		throw Exception("Unknown asset type: " + toString(int(type)));
	}
}

const std::vector<Path>& AssetImporter::getAssetsSrc() const
{
	return assetsSrc;
}
