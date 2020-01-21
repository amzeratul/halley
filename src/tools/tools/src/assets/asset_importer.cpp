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
#include "importers/audio_event_importer.h"
#include "importers/sprite_importer.h"
#include "importers/spritesheet_importer.h"
#include "importers/bitmap_font_importer.h"
#include "importers/shader_importer.h"
#include "halley/text/string_converter.h"
#include "halley/tools/project/project.h"
#include "importers/texture_importer.h"
#include "importers/variable_importer.h"
#include "importers/mesh_importer.h"

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
		std::make_unique<AudioEventImporter>(),
		std::make_unique<SpriteImporter>(),
		std::make_unique<SpriteSheetImporter>(),
		std::make_unique<ShaderImporter>(),
		std::make_unique<TextureImporter>(),
		std::make_unique<MeshImporter>(),
		std::make_unique<IAssetImporter>(),
		std::make_unique<VariableImporter>()
	};

	for (auto& importer: defaultImporters) {
		auto type = importer->getType();
		auto& importerSet = importers[type];
		importerSet.emplace_back(std::move(importer));
		for (auto& pluginImporter: project.getAssetImportersFromPlugins(type)) {
			importerSet.emplace_back(std::move(pluginImporter));
		}
	}
}

IAssetImporter& AssetImporter::getRootImporter(Path path) const
{
	ImportAssetType type = ImportAssetType::SimpleCopy;
	
	const auto root = path.getRoot();
	if (root == "font") {
		type = ImportAssetType::Font;
	} else if (root == "bitmap_font") {
		type = ImportAssetType::BitmapFont;
	} else if (root == "image" || root == "sprite") {
		type = ImportAssetType::Sprite;
	} else if (root == "animation") {
		type = ImportAssetType::Animation;
	} else if (root == "material") {
		type = ImportAssetType::Material;
	} else if (root == "config") {
		type = ImportAssetType::Config;
	} else if (root == "audio") {
		type = ImportAssetType::Audio;
	} else if (root == "audio_event") {
		type = ImportAssetType::AudioEvent;
	} else if (root == "spritesheet") {
		type = ImportAssetType::SpriteSheet;
	} else if (root == "shader") {
		type = ImportAssetType::Skip;
	} else if (root == "texture") {
		type = ImportAssetType::Texture;
	} else if (root == "mesh") {
		type = ImportAssetType::Mesh;
	} else if (root == "variable") {
		type = ImportAssetType::VariableTable;
	}

	return getImporters(type).at(0);
}

std::vector<std::reference_wrapper<IAssetImporter>> AssetImporter::getImporters(ImportAssetType type) const
{
	std::vector<std::reference_wrapper<IAssetImporter>> result;

	auto i = importers.find(type);
	if (i != importers.end()) {
		for (auto& importer: i->second) {
			result.push_back(*importer);
		}
		return result;
	}

	throw Exception("Unknown asset type: " + toString(int(type)), HalleyExceptions::Tools);
}

const std::vector<Path>& AssetImporter::getAssetsSrc() const
{
	return assetsSrc;
}
