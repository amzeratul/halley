#include "audio_event_importer.h"
#include "halley/audio/audio_event.h"
#include <yaml-cpp/yaml.h>
#include "config_importer.h"
#include "halley/tools/yaml/yaml_convert.h"
using namespace Halley;

void AudioEventImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	const auto& data = gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data));
	const String strData(reinterpret_cast<const char*>(data.data()), data.size());
	const YAML::Node yamlRoot = YAML::Load(strData.cppStr());
	const auto root = YAMLConvert::parseYAMLNode(yamlRoot);

	const auto event = AudioEvent(root);
	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::AudioEvent, Serializer::toBytes(event));
}
