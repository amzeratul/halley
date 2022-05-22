#include "config_importer.h"

#include "halley/entity/prefab.h"
#include "halley/file_formats/config_file.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/navigation/navmesh_set.h"

using namespace Halley;

void ConfigImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	ConfigFile config = YAMLConvert::parseConfig(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));
	
	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "deflate");

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::ConfigFile, Serializer::toBytes(config), meta);
}

void PrefabImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Prefab prefab;
	prefab.parseYAML(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));

	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "deflate");

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::Prefab, Serializer::toBytes(prefab, SerializerOptions(SerializerOptions::maxVersion)), meta);
}

void SceneImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Scene scene;
	scene.parseYAML(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));

	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "deflate");

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::Scene, Serializer::toBytes(scene, SerializerOptions(SerializerOptions::maxVersion)), meta);
}

void NavmeshSetImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	ConfigFile config = YAMLConvert::parseConfig(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));
	auto navmeshSet = NavmeshSet(config.getRoot());

	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "deflate");

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::NavmeshSet, Serializer::toBytes(navmeshSet, SerializerOptions(SerializerOptions::maxVersion)), meta);
}