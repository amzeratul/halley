#pragma once
#include "halley/plugin/iasset_importer.h"
#include <gsl/gsl>
#include "halley/file_formats/config_file.h"
#include <yaml-cpp/node/node.h>

namespace Halley
{
	class ConfigFile;

	class ConfigImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Config; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

		static ConfigNode parseYAMLNode(const YAML::Node& node);
		static void parseConfig(ConfigFile& config, gsl::span<const gsl::byte> data);
	};
}
