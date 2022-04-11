#include "audio_object_importer.h"
#include "halley/audio/audio_object.h"
#include "config_importer.h"
#include "halley/file_formats/yaml_convert.h"
using namespace Halley;

void AudioObjectImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	auto config = YAMLConvert::parseConfig(asset.inputFiles.at(0).data);

	const auto event = AudioObject(config.getRoot());
	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::AudioObject, Serializer::toBytes(event));
}
