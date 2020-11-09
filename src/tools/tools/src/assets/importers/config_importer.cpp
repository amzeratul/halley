#include "config_importer.h"

#include "halley/entity/prefab.h"
#include "halley/file_formats/config_file.h"
#include "halley/file_formats/yaml_convert.h"

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
	YAMLConvert::parseConfig(prefab, gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));

	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "deflate");

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::Prefab, Serializer::toBytes(prefab), meta);
}

void SceneImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Scene scene;
	YAMLConvert::parseConfig(scene, gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));

	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "deflate");

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::Scene, Serializer::toBytes(scene), meta);
}

