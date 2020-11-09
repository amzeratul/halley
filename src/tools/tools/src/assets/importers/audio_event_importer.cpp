#include "audio_event_importer.h"
#include "halley/audio/audio_event.h"
#include "config_importer.h"
#include "halley/file_formats/yaml_convert.h"
using namespace Halley;

void AudioEventImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	auto config = YAMLConvert::parseConfig(asset.inputFiles.at(0).data);

	const auto event = AudioEvent(config.getRoot());
	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::AudioEvent, Serializer::toBytes(event));
}
