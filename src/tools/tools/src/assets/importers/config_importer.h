#pragma once
#include "halley/plugin/iasset_importer.h"
#include "halley/file_formats/config_file.h"

namespace Halley
{
	class ConfigFile;

	class ConfigImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::ConfigFile; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};

	class PrefabImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Prefab; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};

	class SceneImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Scene; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
