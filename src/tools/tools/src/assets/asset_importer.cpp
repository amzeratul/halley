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
#include "halley/tools/project/project_properties.h"
#include "importers/texture_importer.h"
#include "importers/variable_importer.h"
#include "importers/mesh_importer.h"
#include "importers/render_graph_importer.h"

using namespace Halley;

AssetImporter::AssetImporter(Project& project, std::vector<Path> assetsSrc)
	: assetsSrc(std::move(assetsSrc))
{
	std::unique_ptr<IAssetImporter> defaultImporters[] = {
		std::make_unique<CopyFileImporter>(ImportAssetType::SimpleCopy, AssetType::BinaryFile),
		std::make_unique<FontImporter>(),
		std::make_unique<BitmapFontImporter>(),
		std::make_unique<ImageImporter>(),
		std::make_unique<AnimationImporter>(),
		std::make_unique<MaterialImporter>(),
		std::make_unique<ConfigImporter>(),
		std::make_unique<PrefabImporter>(),
		std::make_unique<SceneImporter>(),
		std::make_unique<CodegenImporter>(),
		std::make_unique<AudioImporter>(),
		std::make_unique<AudioEventImporter>(),
		std::make_unique<SpriteImporter>(),
		std::make_unique<SpriteSheetImporter>(),
		std::make_unique<ShaderImporter>(),
		std::make_unique<TextureImporter>(),
		std::make_unique<MeshImporter>(),
		std::make_unique<SkipAssetImporter>(),
		std::make_unique<VariableImporter>(),
		std::make_unique<RenderGraphImporter>()
	};

	importByExtension = project.getProperties().getImportByExtension();

	for (auto& importer: defaultImporters) {
		auto type = importer->getType();
		auto& importerSet = importers[type];
		
		addImporter(importerSet, std::move(importer));
		for (auto& pluginImporter: project.getAssetImportersFromPlugins(type)) {
			addImporter(importerSet, std::move(pluginImporter));
		}
	}
}

void AssetImporter::addImporter(std::vector<std::unique_ptr<IAssetImporter>>& dst, std::unique_ptr<IAssetImporter> importer)
{
	importer->setImportByExtension(importByExtension);
	dst.emplace_back(std::move(importer));
}

ImportAssetType AssetImporter::getImportAssetType(const Path& path, bool skipRedundantTypes) const
{
	const auto root = path.getRoot();
	
	if (root == "font") {
		return ImportAssetType::Font;
	} else if (root == "bitmap_font") {
		return ImportAssetType::BitmapFont;
	} else if (root == "image" || root == "sprite") {
		return ImportAssetType::Sprite;
	} else if (root == "animation") {
		return ImportAssetType::Animation;
	} else if (root == "material") {
		return ImportAssetType::MaterialDefinition;
	} else if (root == "config") {
		return ImportAssetType::ConfigFile;
	} else if (root == "prefab") {
		return ImportAssetType::Prefab;
	} else if (root == "scene") {
		return ImportAssetType::Scene;
	} else if (root == "audio") {
		return ImportAssetType::AudioClip;
	} else if (root == "audio_event") {
		return ImportAssetType::AudioEvent;
	} else if (root == "spritesheet") {
		return ImportAssetType::SpriteSheet;
	} else if (root == "shader") {
		return skipRedundantTypes ? ImportAssetType::Skip : ImportAssetType::Shader;
	} else if (root == "texture") {
		return ImportAssetType::Texture;
	} else if (root == "mesh") {
		return ImportAssetType::Mesh;
	} else if (root == "variable") {
		return ImportAssetType::VariableTable;
	} else if (root == "render_graph") {
		return ImportAssetType::RenderGraphDefinition;
	}

	return ImportAssetType::SimpleCopy;
}

IAssetImporter& AssetImporter::getRootImporter(const Path& path) const
{
	return getImporters(getImportAssetType(path, true)).at(0);
}

std::vector<std::reference_wrapper<IAssetImporter>> AssetImporter::getImporters(ImportAssetType type) const
{
	std::vector<std::reference_wrapper<IAssetImporter>> result;

	auto i = importers.find(type);
	if (i != importers.end()) {
		for (const auto& importer: i->second) {
			result.emplace_back(*importer);
		}
		return result;
	}

	throw Exception("Unknown asset type: " + toString(int(type)), HalleyExceptions::Tools);
}

const std::vector<Path>& AssetImporter::getAssetsSrc() const
{
	return assetsSrc;
}
