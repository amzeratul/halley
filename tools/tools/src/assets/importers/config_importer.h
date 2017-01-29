#pragma once
#include "halley/plugin/iasset_importer.h"
#include <gsl/gsl>

namespace Halley
{
	class ConfigFile;

	class ConfigImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Config; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

		static void parseConfig(ConfigFile& config, gsl::span<const gsl::byte> data);
	};
}
